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
#define IMX_CPU_TEMP_RECORDS 1
#define INA_POWER_RECORDS 3

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

    // -- McpManager related --
    DpContainer m_mcpTempContainer; //! Tracked temperature container state
    FwSizeType m_mcpRecordCount;       //!< Count of serialized records
    bool m_mcpTempContainerValid;    //!< Whether the container is valid 

    // -- ImxThermalManager related --
    DpContainer m_cpuTempContainer; //! Tracked temperature container state
    FwSizeType m_cpuRecordCount;       //!< Count of serialized records
    bool m_cpuTempContainerValid;    //!< Whether the container is valid 

    // -- InaManager related --
    DpContainer m_inaPowerContainer; //! Tracked power container state
    FwSizeType m_inaRecordCount;       //!< Count of serialized records
    bool m_inaPowerContainerValid;    //!< Whether the container is valid 
  

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
    
    //! Handler implementation for cpuThermalReadIn
    //!
    //! Input port to receive ImxThermalManager thermal readings
    void cpuThermalReadIn_handler(
        FwIndexType portNum,                                //!< The port number
        const scalesSvc::ThermalReading& cpuThermalReading  //!< Thermal Reading at the IMX CPU
        ) override;
    
    //! Handler implementation for inaPowerReadIn
    //!
    //! Input port to receive InaManager power readings
    void inaPowerReadIn_handler(FwIndexType portNum,                               //!< The port number
                                const scalesSvc::PowerReading& obcPowerReading,    //!< Power Reading of OBC
                                const scalesSvc::PowerReading& perifPowerReading,  //!< Power Reading of Peripheral
                                const scalesSvc::PowerReading& jetsonPowerReading  //!< Power Reading of Jetson
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
    
    bool initCpuContainer(); //! Intialize and allocate memory for imx_cpu container

    bool cpuSerialize_Send(const scalesSvc::ThermalReading& cpuThermalReading); //! serialize and send cpu temp records

    bool initInaContainer(); //! Initialize and allocate memory for ina power container

    bool inaSerialize_Send(const scalesSvc::PowerReading& obcPowerReading,
                           const scalesSvc::PowerReading& perifPowerReading,
                           const scalesSvc::PowerReading& jetsonPowerReading);

  
};


}  // namespace scalesSvc

#endif
