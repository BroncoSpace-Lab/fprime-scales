// ======================================================================
// \title  DataProducer.cpp
// \author bidat
// \brief  cpp file for DataProducer component implementation class
// ======================================================================

#include "scales/scalesSvc/DataProducer/DataProducer.hpp"

namespace scalesSvc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

DataProducer ::DataProducer(const char* const compName) : 
    DataProducerComponentBase(compName),
    m_mcpTempContainer(),
    m_mcpTempContainerValid(false),
    m_mcpRecordCount(0),

    m_cpuTempContainer(),
    m_cpuTempContainerValid(false),
    m_cpuRecordCount(0),

    m_jetsonTempContainer(),
    m_jetsonTempRecordCount(0),
    m_jetsonTempContainerValid(false),

    m_inaPowerContainer(),
    m_inaPowerContainerValid(false),
    m_inaRecordCount(0)
{

}

DataProducer ::~DataProducer() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void DataProducer ::McpThermalReadingIn_handler(FwIndexType portNum,
                                                const scalesSvc::ThermalReading& obcThermalReading,
                                                const scalesSvc::ThermalReading& perifThermalReading,
                                                const scalesSvc::ThermalReading& jetsonThermalReading) {

    if(this->m_mcpTempContainerValid){
        if(!this->mcpSerialize_Send(obcThermalReading, perifThermalReading, jetsonThermalReading)){
            printf("[ERROR] Couldn't serialize and send mcp data products\n");
        }
    }
}

void DataProducer ::cpuThermalReadIn_handler(FwIndexType portNum, const scalesSvc::ThermalReading& cpuThermalReading) {
    if(this->m_cpuTempContainerValid){
        if(!this->cpuSerialize_Send(cpuThermalReading)){
            printf("[ERROR] Couldn't serialize and send cpu data products\n");
        }
    }
}

void DataProducer ::jetsonThermalReadIn_handler(FwIndexType portNum,
                                                const scalesSvc::ThermalReading& jetson_cpuThermalReading,
                                                const scalesSvc::ThermalReading& jetson_gpuTheramlReading,
                                                const scalesSvc::ThermalReading& jetson_cv0ThermalReading,
                                                const scalesSvc::ThermalReading& jetson_cv1ThermalReading,
                                                const scalesSvc::ThermalReading& jetson_cv2ThermalReading,
                                                const scalesSvc::ThermalReading& jetson_soc0ThermalReading,
                                                const scalesSvc::ThermalReading& jetson_soc1ThermalReading,
                                                const scalesSvc::ThermalReading& jetson_soc2ThermalReading,
                                                const scalesSvc::ThermalReading& jetson_tjThermalReading) {
                                                    
    if(this->m_jetsonTempContainerValid){
        if(!this->jetsonTempSerialize_Send( jetson_cpuThermalReading,
                                            jetson_gpuTheramlReading,
                                            jetson_cv0ThermalReading,
                                            jetson_cv1ThermalReading,
                                            jetson_cv2ThermalReading,
                                            jetson_soc0ThermalReading,
                                            jetson_soc1ThermalReading,
                                            jetson_soc2ThermalReading,
                                            jetson_tjThermalReading)){
            printf("[ERROR] Couldn't serialize and send jetson temp zone data products\n");
        }
    }
}

void DataProducer ::inaPowerReadIn_handler(FwIndexType portNum,
                                           const scalesSvc::PowerReading& obcPowerReading,
                                           const scalesSvc::PowerReading& perifPowerReading,
                                           const scalesSvc::PowerReading& jetsonPowerReading) {
    if(this->m_inaPowerContainerValid){
        if(!this->inaSerialize_Send(obcPowerReading, perifPowerReading, jetsonPowerReading)){
            printf("[ERROR] Couldn't serialize and send ina data products\n");
        }
    }
}

void DataProducer ::run_handler(FwIndexType portNum, U32 context) {
    
    if(!this->m_mcpTempContainerValid){
        if(!this->initMcpContainer()){
            printf("[ERROR] Failed to initialize mcp temp container\n");
        }
    }

    if(!this->m_cpuTempContainerValid){
        if(!this->initCpuContainer()){
            printf("[ERROR] Failed to initialize cpu temp container\n");
        }
    }

    if(!this->m_jetsonTempContainerValid){
        if(!this->initJetsonTempContainer()){
            printf("[ERROR] Failed to initialize jetson tmp container\n");
        }
    }

    if(!this->m_inaPowerContainerValid){
        if(!this->initInaContainer()){
            printf("[ERROR] Failed to initialize ina power container\n");
        }
    }
}

bool DataProducer ::initMcpContainer(){
    const FwSizeType MCP_CONTAINER_SIZE = RECORD_COUNT * 
                                          MCP_TEMP_RECORDS * 
                                          (scalesSvc::ThermalReading::SERIALIZED_SIZE + sizeof(FwDpIdType));

    if(this->dpGet_McpTemperatureContainer(MCP_CONTAINER_SIZE, this->m_mcpTempContainer) == Fw::Success::SUCCESS){
        this->m_mcpTempContainerValid = true;
        this->m_mcpTempContainer.setTimeTag(this->getTime());
        printf("Initialized MCP container successfully\n");
        return true;
    }
    
    return false;

}

bool DataProducer ::initCpuContainer(){
    const FwSizeType IMX_CPU_CONTAINER_SIZE = RECORD_COUNT *
                                              IMX_CPU_TEMP_RECORDS *
                                              (scalesSvc::ThermalReading::SERIALIZED_SIZE + sizeof(FwDpIdType));
    
    if(this->dpGet_CpuTemperatureContainer(IMX_CPU_CONTAINER_SIZE, this->m_cpuTempContainer) == Fw::Success::SUCCESS){
        this->m_cpuTempContainerValid = true;
        this->m_cpuTempContainer.setTimeTag(this->getTime());
        printf("Initialized CPU container successfully\n");
        return true;
    }

    return false;

}

bool DataProducer ::initJetsonTempContainer(){
    const FwSizeType JETSON_TEMP_ZONE_CONTAINER_SIZE = RECORD_COUNT *
                                                       JETSON_TEMP_ZONE_RECORDS *
                                                       (scalesSvc::ThermalReading::SERIALIZED_SIZE + sizeof(FwDpIdType)); 
    
    if(this-dpGet_JetsonTemperatureZoneContainer(JETSON_TEMP_ZONE_CONTAINER_SIZE, this->m_jetsonTempContainer)){
        this->m_jetsonTempContainerValid = true;
        this->m_jetsonTempContainer.setTimeTag(this->getTime());
        printf("Initialized Jetson Temp Zone container successfully\n");
        return true;
    }

    return false;
    

}

bool DataProducer ::initInaContainer(){
    const FwSizeType INA_CONTAINER_SIZE = RECORD_COUNT *
                                          INA_POWER_RECORDS *
                                          (scalesSvc::PowerReading::SERIALIZED_SIZE + sizeof(FwDpIdType));

    if(this->dpGet_InaPowerContainer(INA_CONTAINER_SIZE, this->m_inaPowerContainer) == Fw::Success::SUCCESS){
        this->m_inaPowerContainerValid = true;
        this->m_inaPowerContainer.setTimeTag(this->getTime());
        printf("Initalized INA container successfully\n");
        return true;
    }

    return false;
}

bool DataProducer ::mcpSerialize_Send(const scalesSvc::ThermalReading& obcThermalReading,
                           const scalesSvc::ThermalReading& perifThermalReading,
                           const scalesSvc::ThermalReading& jetsonThermalReading){
    
    Fw::SerializeStatus status = this->m_mcpTempContainer.serializeRecord_ImxTemperatureRecord(obcThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
      printf("Error serializing IMX temperature record: %d\n", status);
      return false;
    }

    status = this->m_mcpTempContainer.serializeRecord_PeripheralTemperatureRecord(perifThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
      printf("Error serializing Peripheral temperature record: %d\n", status);
      return false;
    }

    status = this->m_mcpTempContainer.serializeRecord_JetsonTemperatureRecord(jetsonThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
      printf("Error serializing Jetson temperature record: %d\n", status);
      return false;
    }

    this->m_mcpRecordCount++;   

    // If we've reached the record count, send the full product
    if(this->m_mcpRecordCount == RECORD_COUNT){
        this->dpSend(this->m_mcpTempContainer);
        printf("MCP data product sent\n");
        
        // Resets mcp container attributes to be initalizd on next tick
        this->m_mcpRecordCount = 0;
        this->m_mcpTempContainerValid = false;
    }

    return true;

}

bool DataProducer ::cpuSerialize_Send(const scalesSvc::ThermalReading& cpuThermalReading){

    Fw::SerializeStatus status = this->m_cpuTempContainer.serializeRecord_CpuTemperatureRecord(cpuThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error serializing IMX_CPU temperature record: %d\n", status);
        return false;
    }

    this->m_cpuRecordCount++;

    // If we've reached the record count, send the full product
    if(this->m_cpuRecordCount == RECORD_COUNT){
        this->dpSend(this->m_cpuTempContainer);
        printf("IMX_CPU data product sent\n");
        
        // Resets cpu container attributes to be initalizd on next tick
        this->m_cpuRecordCount = 0;
        this->m_cpuTempContainerValid = false;
    }

    return true;

}

bool DataProducer ::jetsonTempSerialize_Send(const scalesSvc::ThermalReading& jetson_cpuThermalReading,  
                                  const scalesSvc::ThermalReading& jetson_gpuTheramlReading,   
                                  const scalesSvc::ThermalReading& jetson_cv0ThermalReading,
                                  const scalesSvc::ThermalReading& jetson_cv1ThermalReading,   
                                  const scalesSvc::ThermalReading& jetson_cv2ThermalReading,   
                                  const scalesSvc::ThermalReading& jetson_soc0ThermalReading,
                                  const scalesSvc::ThermalReading& jetson_soc1ThermalReading,  
                                  const scalesSvc::ThermalReading& jetson_soc2ThermalReading,  
                                  const scalesSvc::ThermalReading& jetson_tjThermalReading){

    Fw::SerializeStatus status = this->m_jetsonTempContainer.serializeRecord_Jetson_CpuTemperatureRecord(jetson_cpuThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing JETSON CPU TEMP READING RECORD\n");
        return false;
    }

    status = this->m_jetsonTempContainer.serializeRecord_Jetson_GpuTemperatureRecord(jetson_gpuTheramlReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing JETSON GPU TEMP READING RECORD\n");
        return false;
    }

    status = this->m_jetsonTempContainer.serializeRecord_Jetson_Cv0TemperatureRecord(jetson_cv0ThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing JETSON CVO TEMP READING RECORD\n");
        return false;
    }

    status = this->m_jetsonTempContainer.serializeRecord_Jetson_Cv1TemperatureRecord(jetson_cv1ThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing JETSON CV1 TEMP READING RECORD\n");
        return false;
    }
    status = this->m_jetsonTempContainer.serializeRecord_Jetson_Cv2TemperatureRecord(jetson_cv2ThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing JETSON CV2 TEMP READING RECORD\n");
        return false;
    }

    status = this->m_jetsonTempContainer.serializeRecord_Jetson_Soc0TemperatureRecord(jetson_soc0ThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing JETSON SOC0 TEMP READING RECORD\n");
        return false;
    }

    status = this->m_jetsonTempContainer.serializeRecord_Jetson_Soc1TemperatureRecord(jetson_soc1ThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing JETSON SOC1 TEMP READING RECORD\n");
        return false;
    }

    status = this->m_jetsonTempContainer.serializeRecord_Jetson_Soc2TemperatureRecord(jetson_soc2ThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing JETSON SOC2 TEMP READING RECORD\n");
        return false;
    }

    status = this->m_jetsonTempContainer.serializeRecord_Jetson_TjTemperatureRecord(jetson_tjThermalReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing JETSON TJ TEMP READING RECORD\n");
        return false;
    }

    this->m_jetsonTempRecordCount++;

    // If we've reached the record count, send the full product
    if(this->m_jetsonTempRecordCount == RECORD_COUNT){
        this->dpSend(this->m_jetsonTempContainer);   
        printf("Jetson Temp Zone products sent!\n");

        // Resets jetson temp container to be initalized on next tick
        this->m_jetsonTempContainerValid = false;
        this->m_jetsonTempRecordCount = 0;
    }

    return true;
}

bool DataProducer ::inaSerialize_Send(const scalesSvc::PowerReading& obcPowerReading,
                           const scalesSvc::PowerReading& perifPowerReading,
                           const scalesSvc::PowerReading& jetsonPowerReading){

    Fw::SerializeStatus status = this->m_inaPowerContainer.serializeRecord_ObcPowerRecord(obcPowerReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) {
        printf("Error Serializing OBC POWER READING RECORD\n");
        return false;
    }

    status = this->m_inaPowerContainer.serializeRecord_PerifPowerRecord(perifPowerReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK){
        printf("Error Serializing PERIF POWER READING RECORD\n");
        return false;
    }

    status = this->m_inaPowerContainer.serializeRecord_JetsonPowerRecord(jetsonPowerReading);
    if (status != Fw::SerializeStatus::FW_SERIALIZE_OK){
        printf("Error Serializing JETSON POWER READING RECORD\n");
        return false;
    }

    this->m_inaRecordCount++;
    
    // If we've reached the record count, send the full product
    if(m_inaRecordCount == RECORD_COUNT){
        this->dpSend(this->m_inaPowerContainer);
        printf("Power Reading Data Product Sent!\n");

        // Resets ina container attributes to be initalizd on next tick
        this->m_inaPowerContainerValid = false;
        this->m_inaRecordCount = 0;
    }

    return true;
}
}  // namespace scalesSvc
