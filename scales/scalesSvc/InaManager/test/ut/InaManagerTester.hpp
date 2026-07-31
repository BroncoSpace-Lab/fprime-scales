// ======================================================================
// \title  InaManagerTester.hpp
// \brief  hpp file for InaManager component test harness implementation class
// ======================================================================

#ifndef scalesSvc_InaManagerTester_HPP
#define scalesSvc_InaManagerTester_HPP

#include "scales/scalesSvc/InaManager/InaManagerGTestBase.hpp"
#include "scales/scalesSvc/InaManager/InaManager.hpp"

#include <map>
#include <utility>
#include <vector>

namespace scalesSvc {

  class InaManagerTester :
    public InaManagerGTestBase
  {

    public:

      // ----------------------------------------------------------------------
      // Constants
      // ----------------------------------------------------------------------

      static const FwSizeType MAX_HISTORY_SIZE = 40;
      static const FwEnumStoreType TEST_INSTANCE_ID = 0;
      static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

    public:

      // ----------------------------------------------------------------------
      // Construction and destruction
      // ----------------------------------------------------------------------

      InaManagerTester();
      ~InaManagerTester();

    public:

      // ----------------------------------------------------------------------
      // Tests
      // ----------------------------------------------------------------------

      //! One full tick with all three sensors succeeding: exercises the
      //! happy path end to end (register read order, unit conversion --
      //! including a 3-decimal truncation case and a negative/signed
      //! current case -- per-sensor location/timestamp/sourceId, telemetry
      //! write, and the inaPowerReadOut fan-out to DataProducer, all from a
      //! single run tick).
      void nominalAllSensorsSucceed();

      //! Calls the three private raw-to-engineering-unit conversion helpers
      //! directly (friend access), independent of the I2C mock: LSB math,
      //! the negative/signed current case, and confirms 3-decimal
      //! truncation (not rounding).
      void conversionHelpersDirect();

      //! Confirms the "just booted" timestamp behavior: the first tick's
      //! reported timestamp is always 0 regardless of the absolute
      //! wall-clock value at boot, and later ticks report elapsed seconds
      //! since that first tick.
      void timestampAdvancesAcrossTicks();

      //! Jetson's VOLTAGE register (the 2nd of 3 read per sensor) fails:
      //! confirms the POWER register is never attempted afterward, the
      //! failure is reported via both I2cReadFailed (with the failing
      //! register/status) and FAIL_TO_READ_TEMP_AT, no telemetry is written
      //! for that one sensor, the other two sensors are unaffected, and
      //! inaPowerReadOut still fires once (partial-failure isolation).
      void singleRegisterFailureStopsSubsequentReadsForThatSensor();

      //! All three sensors fail on their first register: confirms
      //! inaPowerReadOut is still sent exactly once even though nothing was
      //! read successfully this tick, since DataProducer/SSM must still get
      //! a reading to act on (INA-002) rather than silently getting nothing.
      void allSensorsFailStillForwardsToDataProducer();

      //! Peripheral's POWER register (the 3rd and last read per sensor)
      //! fails after CURRENT and VOLTAGE both succeed -- the one failure
      //! point not exercised by the other failure tests, since it's reached
      //! only after two successful reads for the same sensor.
      void powerRegisterFailureAfterCurrentAndVoltageSucceed();

    private:

      // ----------------------------------------------------------------------
      // Helper functions
      // ----------------------------------------------------------------------

      //! Queue a canned raw register value to be returned for (addr, register).
      void setRegister(U32 addr, U8 reg, U16 rawValue);

      //! Force (addr, register) to fail with the given I2C status instead of
      //! returning a canned value.
      void failRegister(U32 addr, U8 reg, Drv::I2cStatus status);

      //! Assert one PowerReading's fields against expected values.
      void assertPowerReading(
          const scalesSvc::PowerReading& reading,
          F32 expVoltage,
          F32 expCurrent,
          F32 expPower,
          U8 expSourceId,
          const char* expLocation,
          U32 expTimestamp
      );

      //! Count how many busWriteRead calls were made for (addr, register).
      //! Reads from m_busWriteReadCalls (captured at call time in
      //! from_busWriteRead_handler), not from the autocoded from-port
      //! history: writeBuffer/readBuffer there only store pointers into
      //! readRegister16's local stack arrays, which are reused by every
      //! subsequent register read within the same tick, so their content is
      //! stale by the time a test inspects it after the fact.
      FwSizeType countBusWriteReadCalls(U32 addr, U8 reg) const;

      //! Override: supplies canned register data (or a forced failure) based
      //! on the I2C address and register byte in writeBuffer, the same way a
      //! real INA260 would respond to a register read.
      Drv::I2cStatus from_busWriteRead_handler(
          FwIndexType portNum,
          U32 addr,
          Fw::Buffer& writeBuffer,
          Fw::Buffer& readBuffer
      ) override;

      //! Connect ports
      void connectPorts();

      //! Initialize components
      void initComponents();

    private:

      // ----------------------------------------------------------------------
      // Member variables
      // ----------------------------------------------------------------------

      //! The component under test
      InaManager component;

      //! Canned register values, keyed by (I2C address, register address)
      std::map<std::pair<U32, U8>, U16> m_fakeRegisterValues;

      //! Forced failures, keyed by (I2C address, register address)
      std::map<std::pair<U32, U8>, Drv::I2cStatus> m_failRegisters;

      //! Every busWriteRead call actually made, captured at call time as
      //! (addr, register) -- see countBusWriteReadCalls().
      std::vector<std::pair<U32, U8>> m_busWriteReadCalls;

  };

}

#endif
