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
    this->clearHistory();

    this->invoke_to_cpuThermalReadIn(0, this->reading(0, 41.0F, "CPU"));
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(0);

    // A later tick retries and succeeds this time.
    this->invoke_to_run(0, 0);
    this->clearHistory();

    this->invoke_to_cpuThermalReadIn(0, this->reading(0, 41.0F, "CPU"));
    this->component.doDispatch();
    ASSERT_PRODUCT_SEND_SIZE(0);  // one reading is not a full batch yet, but no error either
}

// connectPorts()/initComponents() are auto-generated into
// DataProducerTesterHelpers.cpp by UT_AUTO_HELPERS (see CMakeLists.txt) --
// defining them again here would conflict at link time.

}  // namespace scalesSvc
