// ======================================================================
// \title  DataProducerTester.hpp
// \brief  hpp file for DataProducer component test harness implementation class
// ======================================================================

#ifndef scalesSvc_DataProducerTester_HPP
#define scalesSvc_DataProducerTester_HPP

#include "scales/scalesSvc/DataProducer/DataProducer.hpp"
#include "scales/scalesSvc/DataProducer/DataProducerGTestBase.hpp"

namespace scalesSvc {

class DataProducerTester final : public DataProducerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 400;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 20;
    //! Left at the component's default (0) -- sendCmd_* helpers assume opcodes
    //! are based at 0, so setIdBase() to a nonzero value here would desync
    //! command dispatch from the container-id math in productGet_handler().
    static const FwDpIdType ID_BASE = 0;

    DataProducerTester();
    ~DataProducerTester();

    void disabledByDefaultBlocksContainerInit();
    void enableDataProductsInitializesAllContainers();
    void disableDataProductsInvalidatesContainers();
    void mcpReadingIgnoredWhileContainerInvalid();
    void mcpReadingBatchesAndSendsAfterRecordCount();
    void jetsonZoneReadingBatchesAndSendsAfterRecordCount();
    void cpuReadingBatchesAndSendsAfterRecordCount();
    void inaReadingBatchesAndSendsAfterRecordCount();
    void containerReinitializesAfterBatchSend();
    void containerGetFailureLeavesContainerInvalid();

  private:
    // ----------------------------------------------------------------------
    // Handler for the data product get port
    // ----------------------------------------------------------------------
    Fw::Success::T productGet_handler(FwDpIdType id, FwSizeType size, Fw::Buffer& buffer) override;

  private:
    void connectPorts();
    void initComponents();

    //! Enable data products and run one tick so all four containers initialize.
    void enableAndInitializeAllContainers();

    ThermalReading reading(U8 sensorId, F32 temperature, const char* location);
    PowerReading powerReading(U8 sourceId, F32 voltage, F32 current, F32 power, const char* location);

    DataProducer component;

    //! When true, productGet_handler fails (returns FAILURE) for this specific
    //! container id on its next call, then reverts to succeeding.
    FwDpIdType m_failNextGetForId;
    bool m_failNextGet;

    static constexpr FwSizeType BUFFER_SIZE = 65536;
    U8 m_mcpData[BUFFER_SIZE];
    U8 m_cpuData[BUFFER_SIZE];
    U8 m_jetsonData[BUFFER_SIZE];
    U8 m_inaData[BUFFER_SIZE];
};

}  // namespace scalesSvc

#endif
