module scalesSvc {

  @ Minimal passive relay swapped in as CdhCore's `fatalHandler` instance (via
  @ CdhCoreFatalHandlerConfig.fpp) so a FATAL announcement can be routed through
  @ FPManager for graceful shutdown before it reaches the real, process-terminating
  @ Svc.FatalHandler. CdhCore.fpp always connects `events.FatalAnnounce ->
  @ fatalHandler.FatalReceive` internally, and both of those ports allow only a
  @ single connection, so a deployment cannot add a second consumer of
  @ FatalAnnounce directly. This relay's FatalReceive port lets it be adopted as
  @ that single consumer, then re-announces the event on a fresh port that is
  @ free to be wired anywhere the deployment needs.
  passive component FatalRelay {

    @ FATAL event receive port, named to match Svc.FatalHandler.FatalReceive so
    @ this component satisfies CdhCore's internal fatalHandler connection.
    sync input port FatalReceive: Svc.FatalEvent

    @ Re-announces the FATAL event to the next stage in the deployment's fatal path.
    output port fatalOut: Svc.FatalEvent

  }

}
