module scalesSvc {

  @ Adapts GenericHub buffer ports to the context-carrying communications ports
  @ used by the F Prime framer and deframer stack.
  passive component HubComAdapter {

    @ Buffer from GenericHub to be framed and transmitted
    sync input port bufferIn: Fw.BufferSend

    @ Return ownership of a buffer received on bufferIn
    output port bufferInReturn: Fw.BufferSend

    @ Send a hub buffer into the framer
    output port comOut: Svc.ComDataWithContext

    @ Return a buffer after the framer/communications stack is done with it
    sync input port comReturnIn: Svc.ComDataWithContext

    @ Receive a deframed buffer from the communications stack
    guarded input port comIn: Svc.ComDataWithContext

    @ Return ownership of a buffer received on comIn
    output port comInReturn: Svc.ComDataWithContext

    @ Send the deframed buffer into GenericHub
    output port bufferOut: Fw.BufferSend

    @ Return a consumed deframed buffer to the deframer
    sync input port bufferOutReturn: Fw.BufferSend

    @ Real hub link status from the ComStub sitting downstream of this
    @ adapter (Svc.ComStub.comStatusOut). Fanned out unconditionally to
    @ every connected index of comStatusOut below -- F Prime ports are
    @ strictly 1:1, so when more than one consumer needs the same status
    @ (e.g. a status-aware send queue that needs it to actually gate sends,
    @ and JetsonManager, which wants it directly for its own fast rejection
    @ path -- see JM-016 in JetsonManager's SDD), this is the single place
    @ that splits it.
    sync input port comStatusIn: Fw.SuccessCondition

    @ Fan-out of comStatusIn to however many consumers are connected in the
    @ topology (currently 2: the hub-link ComQueue and JetsonManager).
    output port comStatusOut: [2] Fw.SuccessCondition
  }
}
