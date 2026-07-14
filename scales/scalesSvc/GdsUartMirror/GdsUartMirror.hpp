// ======================================================================
// \title  GdsUartMirror.hpp
// \brief  Mirrors the GDS framed byte stream to a UART driver
// ======================================================================

#ifndef scales_scalesSvc_GdsUartMirror_HPP
#define scales_scalesSvc_GdsUartMirror_HPP

#include "scales/scalesSvc/GdsUartMirror/GdsUartMirrorComponentAc.hpp"

#include <Os/Mutex.hpp>

namespace scalesSvc {

class GdsUartMirror final : public GdsUartMirrorComponentBase {
  public:
    explicit GdsUartMirror(const char* const compName);
    ~GdsUartMirror() override;

  private:
    enum class ReceiveSource {
        NONE,
        PRIMARY,
        MIRROR,
    };

    struct ReceiveRecord {
        U8* data;
        U32 context;
        ReceiveSource source;
    };

    static constexpr FwSizeType RECEIVE_RECORD_COUNT = 256;

    void primaryConnectedIn_handler(FwIndexType portNum) override;
    void mirrorConnectedIn_handler(FwIndexType portNum) override;
    Drv::ByteStreamStatus sendIn_handler(FwIndexType portNum, Fw::Buffer& sendBuffer) override;
    void primaryReceiveIn_handler(FwIndexType portNum, Fw::Buffer& buffer, Drv::ByteStreamStatus status) override;
    void mirrorReceiveIn_handler(FwIndexType portNum, Fw::Buffer& buffer, Drv::ByteStreamStatus status) override;
    void receiveReturnIn_handler(FwIndexType portNum, Fw::Buffer& buffer) override;

    void forwardReceive(Fw::Buffer& buffer, Drv::ByteStreamStatus status, ReceiveSource source);
    bool rememberReceiveBuffer(const Fw::Buffer& buffer, ReceiveSource source);
    ReceiveSource forgetReceiveBuffer(const Fw::Buffer& buffer);
    void returnReceiveBuffer(Fw::Buffer& buffer, ReceiveSource source);

    Os::Mutex m_recordsMutex;
    ReceiveRecord m_receiveRecords[RECEIVE_RECORD_COUNT];
    bool m_primaryReady;
    bool m_mirrorReady;
};

}  // namespace scalesSvc

#endif
