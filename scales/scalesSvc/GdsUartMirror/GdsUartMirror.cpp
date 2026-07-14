// ======================================================================
// \title  GdsUartMirror.cpp
// \brief  Mirrors the GDS framed byte stream to a UART driver
// ======================================================================

#include "scales/scalesSvc/GdsUartMirror/GdsUartMirror.hpp"

namespace scalesSvc {

GdsUartMirror::GdsUartMirror(const char* const compName)
    : GdsUartMirrorComponentBase(compName), m_receiveRecords{}, m_primaryReady(false), m_mirrorReady(false) {
    for (FwSizeType i = 0; i < RECEIVE_RECORD_COUNT; ++i) {
        this->m_receiveRecords[i].data = nullptr;
        this->m_receiveRecords[i].context = Fw::Buffer::NO_CONTEXT;
        this->m_receiveRecords[i].source = ReceiveSource::NONE;
    }
}

GdsUartMirror::~GdsUartMirror() = default;

void GdsUartMirror::primaryConnectedIn_handler(FwIndexType portNum) {
    this->m_primaryReady = true;
    this->connectedOut_out(0);
}

void GdsUartMirror::mirrorConnectedIn_handler(FwIndexType portNum) {
    this->m_mirrorReady = true;
}

Drv::ByteStreamStatus GdsUartMirror::sendIn_handler(FwIndexType portNum, Fw::Buffer& sendBuffer) {
    Drv::ByteStreamStatus primaryStatus = Drv::ByteStreamStatus::OTHER_ERROR;

    if (this->m_primaryReady && this->isConnected_primarySendOut_OutputPort(0)) {
        primaryStatus = this->primarySendOut_out(0, sendBuffer);
    }

    if (this->m_mirrorReady && this->isConnected_mirrorSendOut_OutputPort(0)) {
        // This is a best-effort mirror. The primary TCP status is returned to
        // ComStub so the normal GDS connection remains the controlling link.
        (void)this->mirrorSendOut_out(0, sendBuffer);
    }

    return primaryStatus;
}

void GdsUartMirror::primaryReceiveIn_handler(FwIndexType portNum,
                                             Fw::Buffer& buffer,
                                             Drv::ByteStreamStatus status) {
    this->forwardReceive(buffer, status, ReceiveSource::PRIMARY);
}

void GdsUartMirror::mirrorReceiveIn_handler(FwIndexType portNum,
                                            Fw::Buffer& buffer,
                                            Drv::ByteStreamStatus status) {
    this->forwardReceive(buffer, status, ReceiveSource::MIRROR);
}

void GdsUartMirror::receiveReturnIn_handler(FwIndexType portNum, Fw::Buffer& buffer) {
    const ReceiveSource source = this->forgetReceiveBuffer(buffer);
    this->returnReceiveBuffer(buffer, source);
}

void GdsUartMirror::forwardReceive(Fw::Buffer& buffer, Drv::ByteStreamStatus status, ReceiveSource source) {
    if ((buffer.getData() == nullptr) || (buffer.getSize() == 0)) {
        this->returnReceiveBuffer(buffer, source);
        return;
    }

    if (!this->rememberReceiveBuffer(buffer, source)) {
        this->returnReceiveBuffer(buffer, source);
        return;
    }

    if (this->isConnected_receiveOut_OutputPort(0)) {
        this->receiveOut_out(0, buffer, status);
    } else {
        (void)this->forgetReceiveBuffer(buffer);
        this->returnReceiveBuffer(buffer, source);
    }
}

bool GdsUartMirror::rememberReceiveBuffer(const Fw::Buffer& buffer, ReceiveSource source) {
    Os::ScopeLock lock(this->m_recordsMutex);

    for (FwSizeType i = 0; i < RECEIVE_RECORD_COUNT; ++i) {
        ReceiveRecord& record = this->m_receiveRecords[i];
        if (record.source == ReceiveSource::NONE) {
            record.data = buffer.getData();
            record.context = buffer.getContext();
            record.source = source;
            return true;
        }
    }

    return false;
}

GdsUartMirror::ReceiveSource GdsUartMirror::forgetReceiveBuffer(const Fw::Buffer& buffer) {
    Os::ScopeLock lock(this->m_recordsMutex);

    for (FwSizeType i = 0; i < RECEIVE_RECORD_COUNT; ++i) {
        ReceiveRecord& record = this->m_receiveRecords[i];
        if ((record.source != ReceiveSource::NONE) && (record.data == buffer.getData()) &&
            (record.context == buffer.getContext())) {
            const ReceiveSource source = record.source;
            record.data = nullptr;
            record.context = Fw::Buffer::NO_CONTEXT;
            record.source = ReceiveSource::NONE;
            return source;
        }
    }

    return ReceiveSource::NONE;
}

void GdsUartMirror::returnReceiveBuffer(Fw::Buffer& buffer, ReceiveSource source) {
    switch (source) {
        case ReceiveSource::PRIMARY:
            if (this->isConnected_primaryReceiveReturnOut_OutputPort(0)) {
                this->primaryReceiveReturnOut_out(0, buffer);
            }
            break;

        case ReceiveSource::MIRROR:
            if (this->isConnected_mirrorReceiveReturnOut_OutputPort(0)) {
                this->mirrorReceiveReturnOut_out(0, buffer);
            }
            break;

        case ReceiveSource::NONE:
        default:
            break;
    }
}

}  // namespace scalesSvc
