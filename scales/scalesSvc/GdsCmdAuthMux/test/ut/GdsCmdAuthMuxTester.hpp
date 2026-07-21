// ======================================================================
// \title  GdsCmdAuthMuxTester.hpp
// \author luquitolanzi
// \brief  hpp file for GdsCmdAuthMux component test harness implementation class
// ======================================================================

#ifndef scalesSvc_GdsCmdAuthMuxTester_HPP
#define scalesSvc_GdsCmdAuthMuxTester_HPP

#include "scales/scalesSvc/GdsCmdAuthMux/GdsCmdAuthMux.hpp"
#include "scales/scalesSvc/GdsCmdAuthMux/GdsCmdAuthMuxGTestBase.hpp"
#include "Fw/Com/ComBuffer.hpp"

namespace scalesSvc {

class GdsCmdAuthMuxTester final : public GdsCmdAuthMuxGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object GdsCmdAuthMuxTester
    GdsCmdAuthMuxTester();

    //! Destroy object GdsCmdAuthMuxTester
    ~GdsCmdAuthMuxTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    void startupWithTcpUp();
    void startupWithTcpDownAndGracePeriod();
    void tcpRecoveryDuringGracePeriod();
    void commandGatingAndResponseRouting();
    void recoveryAndManualReturnToTcp();
    void malformedCommandsAndFailureRecovery();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    void dispatchAll();
    void setTime(U32 seconds);
    void startWithTcp(bool connected);
    void setTcpStatus(bool connected);
    Fw::ComBuffer buildCommand(FwOpcodeType opcode);
    FwOpcodeType extractForwardedOpcode(FwSizeType index);
    void invokeSwitchToTcp(U32 cmdSeq);

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    GdsCmdAuthMux component;
};

}  // namespace scalesSvc

#endif
