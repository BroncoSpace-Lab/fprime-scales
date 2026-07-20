// ======================================================================
// \title  GdsCmdAuthMuxTester.cpp
// \author luquitolanzi
// \brief  cpp file for GdsCmdAuthMux component test harness implementation class
// ======================================================================

#include "GdsCmdAuthMuxTester.hpp"

#include "Fw/Cmd/CmdPacket.hpp"

namespace scalesSvc {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

GdsCmdAuthMuxTester ::GdsCmdAuthMuxTester()
    : GdsCmdAuthMuxGTestBase("GdsCmdAuthMuxTester", GdsCmdAuthMuxTester::MAX_HISTORY_SIZE), component("GdsCmdAuthMux") {
    this->initComponents();
    this->connectPorts();
}

GdsCmdAuthMuxTester ::~GdsCmdAuthMuxTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void GdsCmdAuthMuxTester ::dispatchAll() {
    // State-machine actions enqueue follow-up signals, so drain until the
    // queue is empty instead of using the generated one-pass helper.
    for (FwSizeType i = 0; i < 100; ++i) {
        const auto status = GdsCmdAuthMuxTesterBase::dispatchCurrentMessages(this->component);
        if (status != GdsCmdAuthMuxComponentBase::MSG_DISPATCH_OK) {
            break;
        }
    }
}

void GdsCmdAuthMuxTester ::setTime(U32 seconds) {
    this->setTestTime(Fw::Time(seconds, 0));
}

void GdsCmdAuthMuxTester ::startWithTcp(bool connected) {
    Fw::Success status = connected ? Fw::Success::SUCCESS : Fw::Success::FAILURE;
    this->invoke_to_tcpGdsStatus(0, status);
    this->setTime(0);
    this->invoke_to_run(0, 0);
    this->dispatchAll();
}

void GdsCmdAuthMuxTester ::setTcpStatus(bool connected) {
    Fw::Success status = connected ? Fw::Success::SUCCESS : Fw::Success::FAILURE;
    this->invoke_to_tcpGdsStatus(0, status);
    this->dispatchAll();
}

Fw::ComBuffer GdsCmdAuthMuxTester ::buildCommand(FwOpcodeType opcode) {
    Fw::ComBuffer data;
    Fw::CmdArgBuffer args;
    EXPECT_EQ(data.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)),
              Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(data.serializeFrom(opcode), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(data.serializeFrom(args), Fw::FW_SERIALIZE_OK);
    return data;
}

void GdsCmdAuthMuxTester ::invokeSwitchToTcp(U32 cmdSeq) {
    Fw::CmdArgBuffer args;
    this->component.get_cmdIn_InputPort(0)->invoke(this->component.getIdBase(), cmdSeq, args);
}

void GdsCmdAuthMuxTester ::startupWithTcpUp() {
    this->startWithTcp(true);
    ASSERT_TLM_CommandAuthority_SIZE(2);
    ASSERT_TLM_CommandAuthority(this->tlmHistory_CommandAuthority->size() - 1, scalesSvc::CommandAuthority::TCP);
    ASSERT_TLM_TcpReadyForAuthority(this->tlmHistory_TcpReadyForAuthority->size() - 1, Fw::On::OFF);
    ASSERT_EVENTS_CommandAuthoritySwitchedToUart_SIZE(0);
    ASSERT_EVENTS_CommandAuthoritySwitchedToTcp_SIZE(0);
}

void GdsCmdAuthMuxTester ::startupWithTcpDownAndGracePeriod() {
    this->startWithTcp(false);
    ASSERT_TLM_CommandAuthority(this->tlmHistory_CommandAuthority->size() - 1, scalesSvc::CommandAuthority::TCP);

    this->setTime(9);
    this->invoke_to_run(0, 9);
    this->dispatchAll();
    ASSERT_EVENTS_CommandAuthoritySwitchedToUart_SIZE(0);

    this->setTime(10);
    this->invoke_to_run(0, 10);
    this->dispatchAll();
    ASSERT_EVENTS_CommandAuthoritySwitchedToUart_SIZE(1);
    ASSERT_TLM_CommandAuthority(this->tlmHistory_CommandAuthority->size() - 1, scalesSvc::CommandAuthority::UART);
}

void GdsCmdAuthMuxTester ::tcpRecoveryDuringGracePeriod() {
    this->startWithTcp(false);
    this->setTime(5);
    this->setTcpStatus(true);
    this->invoke_to_run(0, 5);
    this->dispatchAll();
    ASSERT_EVENTS_TcpRecoveredDuringGrace_SIZE(1);
    ASSERT_EVENTS_CommandAuthoritySwitchedToUart_SIZE(0);
    ASSERT_TLM_CommandAuthority(0, scalesSvc::CommandAuthority::TCP);
}

void GdsCmdAuthMuxTester ::commandGatingAndResponseRouting() {
    this->startWithTcp(true);
    Fw::ComBuffer tcpCommand = this->buildCommand(this->component.getIdBase() + 1);
    this->invoke_to_tcpCmdIn(0, tcpCommand, 10);
    ASSERT_from_cmdOut_SIZE(1);
    ASSERT_from_tcpCmdResponseOut_SIZE(0);

    Fw::ComBuffer inactiveUartCommand = this->buildCommand(this->component.getIdBase() + 2);
    this->invoke_to_uartCmdIn(0, inactiveUartCommand, 11);
    ASSERT_from_uartCmdResponseOut_SIZE(1);
    ASSERT_from_uartCmdResponseOut(0, this->component.getIdBase() + 2, 11, Fw::CmdResponse::BUSY);

    Fw::CmdResponse response = Fw::CmdResponse::OK;
    this->invoke_to_cmdResponseIn(0, this->component.getIdBase() + 1, 10, response);
    ASSERT_from_tcpCmdResponseOut_SIZE(1);
    ASSERT_from_tcpCmdResponseOut(0, this->component.getIdBase() + 1, 10, response);

    this->setTcpStatus(false);
    this->setTime(10);
    this->invoke_to_run(0, 10);
    this->dispatchAll();

    Fw::ComBuffer uartCommand = this->buildCommand(this->component.getIdBase() + 2);
    this->invoke_to_uartCmdIn(0, uartCommand, 20);
    ASSERT_from_cmdOut_SIZE(2);
    Fw::ComBuffer rejectedTcpCommand = this->buildCommand(this->component.getIdBase() + 1);
    this->invoke_to_tcpCmdIn(0, rejectedTcpCommand, 21);
    ASSERT_from_tcpCmdResponseOut_SIZE(2);
    ASSERT_from_tcpCmdResponseOut(1, this->component.getIdBase() + 1, 21, Fw::CmdResponse::BUSY);

    this->invoke_to_cmdResponseIn(0, this->component.getIdBase() + 2, 20, response);
    ASSERT_from_uartCmdResponseOut_SIZE(2);
    ASSERT_from_uartCmdResponseOut(1, this->component.getIdBase() + 2, 20, response);

    Fw::ComBuffer rejectedUartCommand = this->buildCommand(this->component.getIdBase() + 2);
    this->invoke_to_uartCmdIn(0, rejectedUartCommand, 22);
    ASSERT_from_cmdOut_SIZE(3);
    ASSERT_from_uartCmdResponseOut_SIZE(2);
    ASSERT_EVENTS_CommandRejectedInactiveAuthority_SIZE(2);
}

void GdsCmdAuthMuxTester ::recoveryAndManualReturnToTcp() {
    this->startWithTcp(false);
    this->setTime(10);
    this->invoke_to_run(0, 10);
    this->dispatchAll();

    this->setTime(11);
    this->setTcpStatus(true);
    this->invoke_to_run(0, 11);
    this->dispatchAll();
    ASSERT_EVENTS_TcpGdsRecovered_SIZE(1);

    this->setTime(13);
    this->invoke_to_run(0, 13);
    this->dispatchAll();
    ASSERT_TLM_TcpReadyForAuthority(this->tlmHistory_TcpReadyForAuthority->size() - 1, Fw::On::OFF);

    this->setTime(14);
    this->invoke_to_run(0, 14);
    this->dispatchAll();
    ASSERT_EVENTS_TcpGdsStable_SIZE(1);
    ASSERT_TLM_TcpReadyForAuthority(this->tlmHistory_TcpReadyForAuthority->size() - 1, Fw::On::ON);

    Fw::ComBuffer recoveryCommand = this->buildCommand(this->component.getIdBase());
    this->invoke_to_tcpCmdIn(0, recoveryCommand, 30);
    ASSERT_from_cmdOut_SIZE(1);

    Fw::ComBuffer gatedCommand = this->buildCommand(this->component.getIdBase() + 1);
    this->invoke_to_tcpCmdIn(0, gatedCommand, 31);
    ASSERT_from_tcpCmdResponseOut_SIZE(1);
    ASSERT_from_tcpCmdResponseOut(0, this->component.getIdBase() + 1, 31, Fw::CmdResponse::BUSY);

    this->invokeSwitchToTcp(32);
    this->dispatchAll();
    ASSERT_EQ(this->cmdResponseHistory->size(), 1U);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);
    ASSERT_EVENTS_CommandAuthoritySwitchedToTcp_SIZE(1);
    ASSERT_TLM_CommandAuthority(this->tlmHistory_CommandAuthority->size() - 1, scalesSvc::CommandAuthority::TCP);
}

void GdsCmdAuthMuxTester ::malformedCommandsAndFailureRecovery() {
    this->startWithTcp(true);
    Fw::ComBuffer malformed;
    this->invoke_to_uartCmdIn(0, malformed, 40);
    ASSERT_from_uartCmdResponseOut_SIZE(1);
    ASSERT_from_uartCmdResponseOut(0, 0, 40, Fw::CmdResponse::BUSY);

    this->component.gdsMuxStateMachine_sendSignal_failure();
    this->dispatchAll();
    ASSERT_TLM_CommandAuthority(this->tlmHistory_CommandAuthority->size() - 1, scalesSvc::CommandAuthority::TCP);
    ASSERT_TLM_TcpReadyForAuthority(this->tlmHistory_TcpReadyForAuthority->size() - 1, Fw::On::OFF);

    this->setTcpStatus(false);
    this->setTime(10);
    this->invoke_to_run(0, 10);
    this->dispatchAll();
    this->setTcpStatus(true);
    this->setTime(11);
    this->invoke_to_run(0, 11);
    this->dispatchAll();
    this->setTcpStatus(false);
    this->setTime(12);
    this->invoke_to_run(0, 12);
    this->dispatchAll();
    ASSERT_TLM_TcpReadyForAuthority(this->tlmHistory_TcpReadyForAuthority->size() - 1, Fw::On::OFF);
}

}  // namespace scalesSvc
