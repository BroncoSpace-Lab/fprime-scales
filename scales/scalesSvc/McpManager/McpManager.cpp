// ======================================================================
// \title  McpManager.cpp
// \author luquitolanzi
// \brief  cpp file for McpManager component implementation class
// ======================================================================

#include "scales/scalesSvc/McpManager/McpManager.hpp"

namespace scalesSvc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

McpManager ::McpManager(const char* const compName) : McpManagerComponentBase(compName) {}

McpManager ::~McpManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void McpManager ::McpRead_handler(FwIndexType portNum, U32 context) {
    U8 rawImx[2];
    U8 rawPerif[2];
    U8 rawJetson[2];
    U8 regAddr = TEMP_WRITE;

    Fw::Buffer writeBuffer(&regAddr, 1);

    /* Start requesing temp data*/

    // First is Imx
    Fw::Buffer readBuffer(rawImx, 2);
    this->McpWriteRead_out(0, IMXMCP_ADDR, writeBuffer, readBuffer);
    
    U8 UpperByte = rawImx[0] & 0x1F; //clear upper byte flag
    U8 LowerByte = rawImx[1];

    if ((UpperByte & 0x10) == 0x10){ //checking for sign bit (Bit 12)
        UpperByte = UpperByte & 0x0F; //Clear SIGN
        this->IMX_MCP_TEMP = 256 - ((UpperByte * 16.0) + (LowerByte/16.0));
    } else {
        this->IMX_MCP_TEMP = ((UpperByte * 16.0) + (LowerByte/16.0));
     }

    (this->mcp_imx_read).settemperature(this->IMX_MCP_TEMP);
    (this->mcp_imx_read).setsensorId(0);
    (this->mcp_imx_read).setlocation(Fw::String("CPU"));
    (this->mcp_imx_read).settimestamp(this->getTime().getSeconds());
    this->tlmWrite_mcp_imx(this->mcp_imx_read);
}

}  // namespace scalesSvc
