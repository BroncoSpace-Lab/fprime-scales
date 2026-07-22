// ======================================================================
// \title  DataProducer.hpp
// \author bidat
// \brief  hpp file for DataProducer component implementation class
// ======================================================================

#ifndef scalesSvc_DataProducer_HPP
#define scalesSvc_DataProducer_HPP

#include "scales/scalesSvc/DataProducer/DataProducerComponentAc.hpp"

// If we have n record types, then the total number of records in the data product will be n * RECORD_COUNT
constexpr static const FwSizeType RECORD_COUNT = 50;  //!< Number of records of each type in the data product

#define MCP_TEMP_RECORDS 3

namespace scalesSvc {

class DataProducer final : public DataProducerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct DataProducer object
    DataProducer(const char* const compName  //!< The component name
    );

    //! Destroy DataProducer object
    ~DataProducer();
  
  
  private:
    
    /* Implementation-specific members */
    DpContainer m_mcpTempContainer; //! Tracked temperature container state
    FwSizeType m_mcpRecordCount;       //!< Count of serialized records
    bool m_mcpTempContainerValid;    //!< Whether the container is valid 
  

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for McpThermalReadingIn
    //!
    //! Input port to receive McpManager thermal readings
    void McpThermalReadingIn_handler(
        FwIndexType portNum,                                   //!< The port number
        const scalesSvc::ThermalReading& obcThermalReading,    //!< Thermal Reading at OBC
        const scalesSvc::ThermalReading& perifThermalReading,  //!< Thermal Reading at Peripheral
        const scalesSvc::ThermalReading& jetsonThermalReading  //!< Thermal Reading at Jetson
        ) override;

    //! Handler implementation for run
    //!
    //! Port run on rate groups
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;
    
    // ----------------------------------------------------------------------
    // Helper Functions
    // ----------------------------------------------------------------------
    bool initMcpContainer(); //! Intialize and allocate memory for mcp container

    bool mcpSerialize_Send(const scalesSvc::ThermalReading& obcThermalReading,
                           const scalesSvc::ThermalReading& perifThermalReading,
                           const scalesSvc::ThermalReading& jetsonThermalReading); //! serialize and send mcp temp records

  
};


}  // namespace scalesSvc

#endif
