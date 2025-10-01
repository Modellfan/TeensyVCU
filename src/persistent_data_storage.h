#ifndef PERSISTENT_DATA_STORAGE_H
#define PERSISTENT_DATA_STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>

#include "settings.h"

class PersistentDataStorage
{
public:
    struct PersistentData
    {
        float energy_initial_Wh = BMS_INITIAL_REMAINING_WH;
        float measured_capacity_Wh = BMS_INITIAL_REMAINING_WH;
        float ampere_seconds_initial = 0.0f;
        float measured_capacity_Ah = 0.0f;
    };

    PersistentDataStorage()
        : initialized(false),
          has_valid_data(false),
          last_sequence(0U),
          last_slot(kSlotCount - 1U)
    {
    }

    void begin()
    {
        if (initialized)
        {
            return;
        }

        initialized = true;
        scan_slots();
    }

    PersistentData load()
    {
        if (!initialized)
        {
            begin();
        }

        return cached_data;
    }

    void save(const PersistentData &data)
    {
        if (!initialized)
        {
            begin();
        }

        const uint32_t next_sequence = has_valid_data ? (last_sequence + 1U) : 1U;
        const size_t next_slot = has_valid_data ? ((last_slot + 1U) % kSlotCount) : 0U;

        write_slot(next_slot, next_sequence, data);

        cached_data = data;
        has_valid_data = true;
        last_sequence = next_sequence;
        last_slot = next_slot;
    }

private:
    struct RecordHeader
    {
        uint32_t magic;
        uint16_t version;
        uint16_t payload_size;
        uint32_t sequence;
        uint32_t crc;
    };

    static constexpr uint32_t kMagic = 0x54564355UL; // 'TVCU'
    static constexpr uint16_t kVersion = 1U;
    static constexpr size_t kSlotCount = 4U;

    bool initialized;
    bool has_valid_data;
    uint32_t last_sequence;
    size_t last_slot;
    PersistentData cached_data;

    size_t slot_size() const
    {
        return sizeof(RecordHeader) + sizeof(PersistentData);
    }

    void scan_slots()
    {
        has_valid_data = false;
        last_sequence = 0U;
        last_slot = kSlotCount - 1U;

        RecordHeader best_header{};
        PersistentData best_data{};

        for (size_t i = 0; i < kSlotCount; ++i)
        {
            RecordHeader header;
            PersistentData data;

            if (!read_slot(i, header, data))
            {
                continue;
            }

            if (!has_valid_data || header.sequence > best_header.sequence)
            {
                has_valid_data = true;
                best_header = header;
                best_data = data;
                last_slot = i;
            }
        }

        if (has_valid_data)
        {
            cached_data = best_data;
            last_sequence = best_header.sequence;
        }
        else
        {
            cached_data = PersistentData{};
            last_sequence = 0U;
            last_slot = kSlotCount - 1U;
        }
    }

    static uint32_t compute_crc(const RecordHeader &header, const PersistentData &data)
    {
        uint32_t hash = 2166136261UL;

        auto update = [&hash](const uint8_t *bytes, size_t len) {
            for (size_t i = 0; i < len; ++i)
            {
                hash ^= bytes[i];
                hash *= 16777619UL;
            }
        };

        RecordHeader header_copy = header;
        header_copy.crc = 0U;

        update(reinterpret_cast<const uint8_t *>(&header_copy), sizeof(header_copy));
        update(reinterpret_cast<const uint8_t *>(&data), sizeof(data));

        return hash;
    }

    bool read_slot(size_t index, RecordHeader &header, PersistentData &data) const
    {
        const size_t base = index * slot_size();
        EEPROM.get(static_cast<int>(base), header);

        if (header.magic != kMagic)
        {
            return false;
        }

        if (header.version != kVersion)
        {
            return false;
        }

        if (header.payload_size != sizeof(PersistentData))
        {
            return false;
        }

        EEPROM.get(static_cast<int>(base + sizeof(RecordHeader)), data);

        const uint32_t stored_crc = header.crc;
        RecordHeader header_copy = header;
        header_copy.crc = 0U;
        const uint32_t calculated_crc = compute_crc(header_copy, data);

        if (calculated_crc != stored_crc)
        {
            return false;
        }

        return true;
    }

    void write_slot(size_t index, uint32_t sequence, const PersistentData &data)
    {
        const size_t base = index * slot_size();

        RecordHeader header;
        header.magic = kMagic;
        header.version = kVersion;
        header.payload_size = sizeof(PersistentData);
        header.sequence = sequence;
        header.crc = 0U;
        header.crc = compute_crc(header, data);

        EEPROM.put(static_cast<int>(base), header);
        EEPROM.put(static_cast<int>(base + sizeof(RecordHeader)), data);
    }
};

#endif // PERSISTENT_DATA_STORAGE_H
