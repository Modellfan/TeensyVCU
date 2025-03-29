:::mermaid
stateDiagram
    [*] --> Init: PowerOn / Reset
    Init --> Run: InitComplete
    Init --> ErrorState: InitErrorDetected
    Run --> PostRun: RunComplete
    Run --> ErrorState: RunErrorDetected
    PostRun --> PrepareShutdown: PostRunComplete
    PrepareShutdown --> WaitForNvM: PrepareShutdownComplete
    WaitForNvM --> Shutdown: NvMComplete
    WaitForNvM --> ErrorState: NvMErrorDetected
    Shutdown --> [*]: ShutdownComplete
    Shutdown --> ErrorState: ShutdownErrorDetected
    Run --> Sleep: SleepRequest
    Sleep --> Wakeup: WakeupEvent
    Sleep --> ErrorState: SleepErrorDetected
    Wakeup --> Run: WakeupComplete
    Wakeup --> ErrorState: WakeupErrorDetected

    state Init {
        [*] --> Initializing
        Initializing --> InitComplete: InitializationSuccess
        Initializing --> ErrorState: InitializationFailure
    }

    state Run {
        [*] --> NormalOperation
        NormalOperation --> ErrorState: OperationErrorDetected
    }

    state PostRun {
        [*] --> PreparingPostRun
        PreparingPostRun --> PostRunComplete: PostRunSuccess
        PreparingPostRun --> ErrorState: PostRunFailure
    }

    state PrepareShutdown {
        [*] --> PreparingShutdown
        PreparingShutdown --> PrepareShutdownComplete: ShutdownPreparationSuccess
        PreparingShutdown --> ErrorState: ShutdownPreparationFailure
    }

    state WaitForNvM {
        [*] --> WaitingForNvM
        WaitingForNvM --> NvMComplete: NvMSuccess
        WaitingForNvM --> ErrorState: NvMFailure
    }

    state Shutdown {
        [*] --> ShuttingDown
        ShuttingDown --> ShutdownComplete: ShutdownSuccess
        ShuttingDown --> ErrorState: ShutdownFailure
    }

    state Sleep {
        [*] --> EnteringSleep
        EnteringSleep --> SleepMode: SleepSuccess
        EnteringSleep --> ErrorState: SleepFailure
    }

    state Wakeup {
        [*] --> WakingUp
        WakingUp --> WakeupComplete: WakeupSuccess
        WakingUp --> ErrorState: WakeupFailure
    }

    state ErrorState {
        [*] --> HandlingError
        HandlingError --> RecoveryAttempt: AttemptRecovery
        HandlingError --> SafeState: EnsureSafety
    }
    
    state SafeState {
        [*] --> SafeMode
        SafeMode --> [*]: SystemReset / PowerOff
    }

    state RecoveryAttempt {
        [*] --> AttemptingRecovery
        AttemptingRecovery --> Init: RecoverySuccess
        AttemptingRecovery --> SafeState: RecoveryFailure
    }
:::
