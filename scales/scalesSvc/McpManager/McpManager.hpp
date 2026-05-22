// ======================================================================
// \title  McpManager.hpp
// \author luquitolanzi
// \brief  hpp file for McpManager component implementation class
// ======================================================================

#ifndef scalesSvc_McpManager_HPP
#define scalesSvc_McpManager_HPP

#include "scales/scalesSvc/McpManager/McpManagerComponentAc.hpp"

namespace scalesSvc {

class McpManager : public McpManagerComponentBase {
  public:
    // Device addresses
    static const U8 IMXMCP_ADDR = 0x19;
    static const U8 PERIFMCP_ADDR = 0x1A;
    static const U8 JETSONMCP_ADDR = 0x1B;

    // Write to temperature register
    static const U8 TEMP_WRITE = 0x05;

    // Temperature variables
    F32 IMX_MCP_TEMP;
    F32 PERIF_MCP_TEMP;
    F32 JETSON_MCP_TEMP;
  

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct McpManager object
    McpManager(const char* const compName  //!< The component name
    );

    //! Destroy McpManager object
    ~McpManager();

  PRIVATE:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for McpRead
    //!
    //! Input port that will poll the sensors for temperature logging
    void McpRead_handler(FwIndexType portNum,  //!< The port number
                         U32 context           //!< The call order
                         ) override;
    scalesSvc::ThermalReading mcp_imx_read;
    scalesSvc::ThermalReading mcp_perif_read;
    scalesSvc::ThermalReading mcp_jetson_read; 
    //Calling the ThermalReading struct for each mcp to be called in the CPP file
};

}  // namespace scalesSvc

#endif
