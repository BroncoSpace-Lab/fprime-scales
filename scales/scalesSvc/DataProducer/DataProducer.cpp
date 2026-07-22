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
    m_mcpRecordCount(0)
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
            printf("[ERROR] Couldn't serialize and send data products");
        }
    }
}

void DataProducer ::run_handler(FwIndexType portNum, U32 context) {
    
    if(!this->m_mcpTempContainerValid){
        if(!this->initMcpContainer()){
            printf("[ERROR] Failed to initialize mcp temp container");
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
        printf("Initialized MCP container successfully");
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
        printf("MCP data product sent");
        
        // Resets mcp container attributes to be initalizd on next tick
        this->m_mcpRecordCount = 0;
        this->m_mcpTempContainerValid = false;
    }

    return true;

}

}  // namespace scalesSvc
