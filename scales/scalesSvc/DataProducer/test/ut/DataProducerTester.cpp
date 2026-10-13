// ======================================================================
// \title  DataProducerTester.cpp
// \brief  cpp file for DataProducer component test harness implementation class
// ======================================================================

#include "DataProducerTester.hpp"

namespace scalesSvc {

DataProducerTester ::DataProducerTester()
    : DataProducerGTestBase("DataProducerTester", DataProducerTester::MAX_HISTORY_SIZE),
      component("DataProducer"),
      m_failNextGetForId(0),
      m_failNextGet(false),
      m_failAllGets(false),
      m_limitCapacity(false),
      m_recordCapacity(0),
      m_mcpData{},
      m_cpuData{},
      m_jetsonData{},
      m_inaData{} {
    this->initComponents();
    this->connectPorts();
}

DataProducerTester ::~DataProducerTester() {
    this->component.deinit();
}

Fw::Success::T DataProducerTester ::productGet_handler(FwDpIdType id, FwSizeType size, Fw::Buffer& buffer) {
    this->pushProductGetEntry(id, size);

    FW_ASSERT(id >= ID_BASE, static_cast<FwAssertArgType>(id), static_cast<FwAssertArgType>(ID_BASE));
    const FwDpIdType localId = id - ID_BASE;

    if (m_failNextGet && localId == m_failNextGetForId) {
        m_failNextGet = false;
        return Fw::Success::FAILURE;
    }

    if (m_failAllGets) {
        return Fw::Success::FAILURE;
    }

    if (m_limitCapacity) {
        size = Fw::DpContainer::MIN_PACKET_SIZE + m_recordCapacity;
    }

    switch (localId) {
        case DataProducer::ContainerId::McpTemperatureContainer:
            FW_ASSERT(size <= sizeof(this->m_mcpData), static_cast<FwAssertArgType>(size));
            buffer = Fw::Buffer(this->m_mcpData, size);
            return Fw::Success::SUCCESS;
        case DataProducer::ContainerId::CpuTemperatureContainer:
            FW_ASSERT(size <= sizeof(this->m_cpuData), static_cast<FwAssertArgType>(size));
            buffer = Fw::Buffer(this->m_cpuData, size);
            return Fw::Success::SUCCESS;
        case DataProducer::ContainerId::JetsonTemperatureZoneContainer:
            FW_ASSERT(size <= sizeof(this->m_jetsonData), static_cast<FwAssertArgType>(size));
            buffer = Fw::Buffer(this->m_jetsonData, size);
            return Fw::Success::SUCCESS;
        case DataProducer::ContainerId::InaPowerContainer:
            FW_ASSERT(size <= sizeof(this->m_inaData), static_cast<FwAssertArgType>(size));
            buffer = Fw::Buffer(this->m_inaData, size);
            return Fw::Success::SUCCESS;
        default:
            return Fw::Success::FAILURE;
    }
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void DataProducerTester ::enableAndInitializeAllContainers() {
    this->sendCmd_ENABLE_DATA_PRODUCTS(0, 1);
    this->invoke_to_run(0, 0);
}

void DataProducerTester ::enableWithRecordCapacity(FwSizeType recordBytes) {
    m_limitCapacity = true;
    m_recordCapacity = recordBytes;
    this->enableAndInitializeAllContainers();
    this->clearHistory();
}

ThermalReading DataProducerTester ::reading(U8 sensorId, F32 temperature, const char* location) {
    ThermalReading r;
    r.set_sensorId(sensorId);
    r.set_temperature(temperature);
    r.set_tempState(scalesSvc::ThermalStates::IDLE);
    r.set_location(Fw::String(location));
    r.set_timestamp(0);
    return r;
}

PowerReading DataProducerTester ::powerReading(U8 sourceId, F32 voltage, F32 current, F32 power, const char* location) {
    PowerReading r;
    r.set_sourceId(sourceId);
    r.set_voltage(voltage);
    r.set_current(current);
    r.set_power(power);
    r.set_location(Fw::String(location));
    r.set_timestamp(0);
    return r;
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void DataProducerTester ::disabledByDefaultBlocksContainerInit() {
    // Data product collection defaults to OFF -- a run tick with no
    // ENABLE_DATA_PRODUCTS first must not request any container.
    this->invoke_to_run(0, 0);
    ASSERT_PRODUCT_GET_SIZE(0);
}

void DataProducerTester ::enableDataProductsInitializesAllContainers() {
    this->sendCmd_ENABLE_DATA_PRODUCTS(0, 1);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);

    this->invoke_to_run(0, 0);

    // All four containers are requested once enabled.
    ASSERT_PRODUCT_GET_SIZE(4);
}

void DataProducerTester ::disableDataProductsInvalidatesContainers() {
    this->enableAndInitializeAllContainers();
    this->clearHistory();

    this->sendCmd_DISABLE_DATA_PRODUCTS(0, 2);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_EQ(this->cmdResponseHistory->at(0).response, Fw::CmdResponse::OK);

    // A reading that arrives after disabling must not be serialized/sent --
    // the container was invalidated, and run_handler no longer reinitializes
    // it since m_dpCollectMode is now false.
    this->invoke_to_cpuThermalReadIn(0, this->reading(0, 42.0F, "CPU"));
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(0);

    this->invoke_to_run(0, 0);
    ASSERT_PRODUCT_GET_SIZE(0);
}

void DataProducerTester ::mcpReadingIgnoredWhileContainerInvalid() {
    // No ENABLE_DATA_PRODUCTS call at all -- the container is never valid.
    this->invoke_to_McpThermalReadingIn(0, this->reading(1, 20.0F, "OBC"), this->reading(2, 21.0F, "PERIPHERAL"),
                                        this->reading(0, 22.0F, "JETSON"));
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(0);
}

void DataProducerTester ::mcpReadingBatchesAndSendsAfterRecordCount() {
    this->enableAndInitializeAllContainers();
    this->clearHistory();

    for (FwSizeType i = 0; i < RECORD_COUNT - 1; i++) {
        this->invoke_to_McpThermalReadingIn(0, this->reading(1, 20.0F, "OBC"), this->reading(2, 21.0F, "PERIPHERAL"),
                                            this->reading(0, 22.0F, "JETSON"));
        this->component.doDispatch();
    }
    ASSERT_PRODUCT_SEND_SIZE(0);

    this->invoke_to_McpThermalReadingIn(0, this->reading(1, 20.0F, "OBC"), this->reading(2, 21.0F, "PERIPHERAL"),
                                        this->reading(0, 22.0F, "JETSON"));
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(1);
    ASSERT_EQ(this->productSendHistory->at(0).id, ID_BASE + DataProducer::ContainerId::McpTemperatureContainer);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_DP_PRODUCT_SENT_SIZE(1);
    ASSERT_EVENTS_DP_PRODUCT_SENT(0, scalesSvc::DpProduct::MCP_TEMP);
}

void DataProducerTester ::jetsonZoneReadingBatchesAndSendsAfterRecordCount() {
    // Regression test for a fixed typo (`this-dpGet_...` instead of
    // `this->dpGet_...`) that made initJetsonTempContainer() fail to compile
    // for any real target -- this is the only test that exercises the
    // 9-argument jetsonThermalReadIn/JetsonTemperatureZoneContainer path.
    this->enableAndInitializeAllContainers();
    this->clearHistory();

    const ThermalReading z = this->reading(0, 30.0F, "ZONE");
    for (FwSizeType i = 0; i < RECORD_COUNT - 1; i++) {
        this->invoke_to_jetsonThermalReadIn(0, z, z, z, z, z, z, z, z, z);
        this->component.doDispatch();
    }
    ASSERT_PRODUCT_SEND_SIZE(0);

    this->invoke_to_jetsonThermalReadIn(0, z, z, z, z, z, z, z, z, z);
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(1);
    ASSERT_EQ(this->productSendHistory->at(0).id, ID_BASE + DataProducer::ContainerId::JetsonTemperatureZoneContainer);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_DP_PRODUCT_SENT_SIZE(1);
    ASSERT_EVENTS_DP_PRODUCT_SENT(0, scalesSvc::DpProduct::JETSON_TEMP);
}

void DataProducerTester ::cpuReadingBatchesAndSendsAfterRecordCount() {
    this->enableAndInitializeAllContainers();
    this->clearHistory();

    for (FwSizeType i = 0; i < RECORD_COUNT - 1; i++) {
        this->invoke_to_cpuThermalReadIn(0, this->reading(0, 41.0F, "CPU"));
        this->component.doDispatch();
    }
    ASSERT_PRODUCT_SEND_SIZE(0);

    this->invoke_to_cpuThermalReadIn(0, this->reading(0, 41.0F, "CPU"));
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(1);
    ASSERT_EQ(this->productSendHistory->at(0).id, ID_BASE + DataProducer::ContainerId::CpuTemperatureContainer);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_DP_PRODUCT_SENT_SIZE(1);
    ASSERT_EVENTS_DP_PRODUCT_SENT(0, scalesSvc::DpProduct::CPU_TEMP);
}

void DataProducerTester ::inaReadingBatchesAndSendsAfterRecordCount() {
    this->enableAndInitializeAllContainers();
    this->clearHistory();

    for (FwSizeType i = 0; i < RECORD_COUNT - 1; i++) {
        this->invoke_to_inaPowerReadIn(0, this->powerReading(0x41, 5.0F, 0.1F, 0.5F, "OBC"),
                                       this->powerReading(0x45, 5.0F, 0.2F, 1.0F, "PERIPHERAL"),
                                       this->powerReading(0x40, 5.0F, 0.3F, 1.5F, "JETSON"));
        this->component.doDispatch();
    }
    ASSERT_PRODUCT_SEND_SIZE(0);

    this->invoke_to_inaPowerReadIn(0, this->powerReading(0x41, 5.0F, 0.1F, 0.5F, "OBC"),
                                   this->powerReading(0x45, 5.0F, 0.2F, 1.0F, "PERIPHERAL"),
                                   this->powerReading(0x40, 5.0F, 0.3F, 1.5F, "JETSON"));
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(1);
    ASSERT_EQ(this->productSendHistory->at(0).id, ID_BASE + DataProducer::ContainerId::InaPowerContainer);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_DP_PRODUCT_SENT_SIZE(1);
    ASSERT_EVENTS_DP_PRODUCT_SENT(0, scalesSvc::DpProduct::INA_POWER);
}

void DataProducerTester ::containerReinitializesAfterBatchSend() {
    this->enableAndInitializeAllContainers();
    this->clearHistory();

    for (FwSizeType i = 0; i < RECORD_COUNT; i++) {
        this->invoke_to_cpuThermalReadIn(0, this->reading(0, 41.0F, "CPU"));
        this->component.doDispatch();
    }
    ASSERT_PRODUCT_SEND_SIZE(1);

    // After a batch completes, only the CPU container is invalid again (the
    // other three are still mid-batch/valid); the next run tick must
    // re-request just that one so a fresh batch can start.
    this->invoke_to_run(0, 0);
    ASSERT_PRODUCT_GET_SIZE(1);
    bool sawCpuContainerRequest = false;
    for (FwSizeType i = 0; i < this->productGetHistory->size(); i++) {
        if (this->productGetHistory->at(i).id == ID_BASE + DataProducer::ContainerId::CpuTemperatureContainer) {
            sawCpuContainerRequest = true;
        }
    }
    ASSERT_TRUE(sawCpuContainerRequest);
}

void DataProducerTester ::containerGetFailureLeavesContainerInvalid() {
    // If the DataProducts subtopology can't allocate a buffer for a
    // container, that container stays invalid and readings for it are
    // dropped (not queued or retried) until a later run tick succeeds.
    m_failNextGet = true;
    m_failNextGetForId = DataProducer::ContainerId::CpuTemperatureContainer;

    this->sendCmd_ENABLE_DATA_PRODUCTS(0, 1);
    this->invoke_to_run(0, 0);
    // Only the failed container reports, the other three initialized fine.
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_DP_CONTAINER_INIT_FAILED_SIZE(1);
    ASSERT_EVENTS_DP_CONTAINER_INIT_FAILED(0, scalesSvc::DpProduct::CPU_TEMP);
    this->clearHistory();

    this->invoke_to_cpuThermalReadIn(0, this->reading(0, 41.0F, "CPU"));
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(0);

    // A later tick retries and succeeds this time.
    this->invoke_to_run(0, 0);
    ASSERT_EVENTS_SIZE(0);
    this->clearHistory();

    this->invoke_to_cpuThermalReadIn(0, this->reading(0, 41.0F, "CPU"));
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(0);  // one reading is not a full batch yet, but no error either
}

void DataProducerTester ::containerGetFailureEventIsThrottled() {
    m_failAllGets = true;

    // First tick: one DP_CONTAINER_INIT_FAILED per product, in run_handler order.
    this->sendCmd_ENABLE_DATA_PRODUCTS(0, 1);
    this->invoke_to_run(0, 0);
    ASSERT_EVENTS_SIZE(4);
    ASSERT_EVENTS_DP_CONTAINER_INIT_FAILED_SIZE(4);
    ASSERT_EVENTS_DP_CONTAINER_INIT_FAILED(0, scalesSvc::DpProduct::MCP_TEMP);
    ASSERT_EVENTS_DP_CONTAINER_INIT_FAILED(1, scalesSvc::DpProduct::CPU_TEMP);
    ASSERT_EVENTS_DP_CONTAINER_INIT_FAILED(2, scalesSvc::DpProduct::JETSON_TEMP);
    ASSERT_EVENTS_DP_CONTAINER_INIT_FAILED(3, scalesSvc::DpProduct::INA_POWER);

    // The event is throttled at 5, so persistent failure adds only one more.
    for (U32 i = 0; i < 3; i++) {
        this->invoke_to_run(0, 0);
    }
    ASSERT_EVENTS_DP_CONTAINER_INIT_FAILED_SIZE(5);
    ASSERT_PRODUCT_GET_SIZE(16);  // every tick still retries all four containers

    // Re-enabling collection re-arms the throttle.
    this->clearHistory();
    this->sendCmd_ENABLE_DATA_PRODUCTS(0, 2);
    this->invoke_to_run(0, 0);
    ASSERT_EVENTS_DP_CONTAINER_INIT_FAILED_SIZE(4);
}

void DataProducerTester ::mcpSerializeFailureLogsEvent(FwSizeType failIndex) {
    static const char* const RECORDS[] = {"ImxTemperatureRecord", "PeripheralTemperatureRecord",
                                          "JetsonTemperatureRecord"};
    FW_ASSERT(failIndex < FW_NUM_ARRAY_ELEMENTS(RECORDS), static_cast<FwAssertArgType>(failIndex));
    const ThermalReading r[] = {this->reading(1, 20.0F, "OBC"), this->reading(2, 21.0F, "PERIPHERAL"),
                                this->reading(0, 22.0F, "JETSON")};
    // Room for exactly failIndex records, so record number failIndex fails.
    // Records hold a string, so their size on the wire is dynamic.
    FwSizeType capacity = 0;
    for (FwSizeType i = 0; i < failIndex; i++) {
        capacity += sizeof(FwDpIdType) + r[i].serializedSize();
    }
    this->enableWithRecordCapacity(capacity);

    this->invoke_to_McpThermalReadingIn(0, r[0], r[1], r[2]);
    this->component.doDispatch();

    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_DP_RECORD_SERIALIZE_FAILED_SIZE(1);
    ASSERT_EVENTS_DP_RECORD_SERIALIZE_FAILED(0, scalesSvc::DpProduct::MCP_TEMP, RECORDS[failIndex],
                                             static_cast<I32>(Fw::FW_SERIALIZE_NO_ROOM_LEFT));
}

void DataProducerTester ::cpuSerializeFailureLogsEvent() {
    this->enableWithRecordCapacity(0);

    this->invoke_to_cpuThermalReadIn(0, this->reading(0, 41.0F, "CPU"));
    this->component.doDispatch();

    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_DP_RECORD_SERIALIZE_FAILED_SIZE(1);
    ASSERT_EVENTS_DP_RECORD_SERIALIZE_FAILED(0, scalesSvc::DpProduct::CPU_TEMP, "CpuTemperatureRecord",
                                             static_cast<I32>(Fw::FW_SERIALIZE_NO_ROOM_LEFT));
}

void DataProducerTester ::jetsonSerializeFailureLogsEvent(FwSizeType failIndex) {
    static const char* const RECORDS[] = {
        "Jetson_CpuTemperatureRecord",  "Jetson_GpuTemperatureRecord",  "Jetson_Cv0TemperatureRecord",
        "Jetson_Cv1TemperatureRecord",  "Jetson_Cv2TemperatureRecord",  "Jetson_Soc0TemperatureRecord",
        "Jetson_Soc1TemperatureRecord", "Jetson_Soc2TemperatureRecord", "Jetson_TjTemperatureRecord"};
    FW_ASSERT(failIndex < FW_NUM_ARRAY_ELEMENTS(RECORDS), static_cast<FwAssertArgType>(failIndex));
    const ThermalReading z = this->reading(0, 30.0F, "ZONE");
    this->enableWithRecordCapacity(failIndex * (sizeof(FwDpIdType) + z.serializedSize()));

    this->invoke_to_jetsonThermalReadIn(0, z, z, z, z, z, z, z, z, z);
    this->component.doDispatch();

    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_DP_RECORD_SERIALIZE_FAILED_SIZE(1);
    ASSERT_EVENTS_DP_RECORD_SERIALIZE_FAILED(0, scalesSvc::DpProduct::JETSON_TEMP, RECORDS[failIndex],
                                             static_cast<I32>(Fw::FW_SERIALIZE_NO_ROOM_LEFT));
}

void DataProducerTester ::inaSerializeFailureLogsEvent(FwSizeType failIndex) {
    static const char* const RECORDS[] = {"ObcPowerRecord", "PerifPowerRecord", "JetsonPowerRecord"};
    FW_ASSERT(failIndex < FW_NUM_ARRAY_ELEMENTS(RECORDS), static_cast<FwAssertArgType>(failIndex));
    const PowerReading r[] = {this->powerReading(0x41, 5.0F, 0.1F, 0.5F, "OBC"),
                              this->powerReading(0x45, 5.0F, 0.2F, 1.0F, "PERIPHERAL"),
                              this->powerReading(0x40, 5.0F, 0.3F, 1.5F, "JETSON")};
    FwSizeType capacity = 0;
    for (FwSizeType i = 0; i < failIndex; i++) {
        capacity += sizeof(FwDpIdType) + r[i].serializedSize();
    }
    this->enableWithRecordCapacity(capacity);

    this->invoke_to_inaPowerReadIn(0, r[0], r[1], r[2]);
    this->component.doDispatch();

    ASSERT_PRODUCT_SEND_SIZE(0);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_DP_RECORD_SERIALIZE_FAILED_SIZE(1);
    ASSERT_EVENTS_DP_RECORD_SERIALIZE_FAILED(0, scalesSvc::DpProduct::INA_POWER, RECORDS[failIndex],
                                             static_cast<I32>(Fw::FW_SERIALIZE_NO_ROOM_LEFT));
}

// connectPorts()/initComponents() are auto-generated into
// DataProducerTesterHelpers.cpp by UT_AUTO_HELPERS (see CMakeLists.txt) --
// defining them again here would conflict at link time.

}  // namespace scalesSvc
