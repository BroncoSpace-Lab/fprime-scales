// ======================================================================
// \title  InaManagerTester.hpp
// \author jacob
// \brief  hpp file for InaManager component test harness implementation class
// ======================================================================

#ifndef scalesSvc_InaManagerTester_HPP
#define scalesSvc_InaManagerTester_HPP

#include "scales/scalesSvc/InaManager/InaManagerGTestBase.hpp"
#include "scales/scalesSvc/InaManager/InaManager.hpp"

#include <deque>

namespace scalesSvc {

  class InaManagerTester :
    public InaManagerGTestBase
  {

    public:

      // ----------------------------------------------------------------------
      // Constants
      // ----------------------------------------------------------------------

      // Maximum size of histories storing events, telemetry, and port outputs
      static const FwSizeType MAX_HISTORY_SIZE = 10;

      // Instance ID supplied to the component instance under test
      static const FwEnumStoreType TEST_INSTANCE_ID = 0;

      // Queue depth supplied to the component instance under test
      static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

    public:

      // ----------------------------------------------------------------------
      // Construction and destruction
      // ----------------------------------------------------------------------

      //! Construct object InaManagerTester
      InaManagerTester();

      //! Destroy object InaManagerTester
      ~InaManagerTester();

    public:

      // ----------------------------------------------------------------------
      // Tests
      // ----------------------------------------------------------------------

      void testNominalReadAllSensors();

      void testI2cFailureSkipsFailedSensorAndLogsEvent();

    private:

      // ----------------------------------------------------------------------
      // Helper functions
      // ----------------------------------------------------------------------

      //! Connect ports
      void connectPorts();

      //! Initialize components
      void initComponents();

      // ----------------------------------------------------------------------
      // Mock I2C read response
      // ----------------------------------------------------------------------

      struct I2cReadResponse {
        U32 sensorAddress;
        U8 registerAddress;
        U16 rawValue;
        Drv::I2cStatus status;
      };

      std::deque<I2cReadResponse> m_i2cResponses;

      // ----------------------------------------------------------------------
      // Helper methods
      // ----------------------------------------------------------------------

      void enqueueSuccessfulSensorRead(
        U32 sensorAddress,
        U16 rawCurrent,
        U16 rawVoltage,
        U16 rawPower
      );

      void enqueueI2cResponse(
        U32 sensorAddress,
        U8 registerAddress,
        U16 rawValue,
        Drv::I2cStatus status = Drv::I2cStatus::I2C_OK
      );

      void invokeRunAndDispatch(U32 context = 0);

      PowerReading makeExpectedReading(
        F32 current,
        F32 voltage,
        F32 power,
        U32 sourceId,
        U32 timestamp
      );

      // ----------------------------------------------------------------------
      // From-port handlers
      // ----------------------------------------------------------------------

      Drv::I2cStatus from_busWriteRead_handler(
        FwIndexType portNum,
        U32 addr,
        Fw::Buffer& writeBuffer,
        Fw::Buffer& readBuffer
      ) override;

    private:

      // ----------------------------------------------------------------------
      // Member variables
      // ----------------------------------------------------------------------

      //! The component under test
      InaManager component;

      // ----------------------------------------------------------------------
      // Constants
      // ----------------------------------------------------------------------

      static constexpr U32 QUEUE_DEPTH = 10;
      static constexpr U32 INSTANCE = 0;

      static constexpr U32 JETSON_ADDR = 0x40;
      static constexpr U32 OBC_ADDR = 0x41;
      static constexpr U32 PERIPHERAL_ADDR = 0x45;

      static constexpr U8 REG_CURRENT = 0x01;
      static constexpr U8 REG_VOLTAGE = 0x02;
      static constexpr U8 REG_POWER = 0x03;

  };

}

#endif
