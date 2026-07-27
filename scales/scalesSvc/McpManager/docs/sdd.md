# Components::McpManager

Functional Description: Component for F Prime FSW Framework

## Usage Examples

The McpManager will perform i2c read write operations to the three onboard MCP9808T sensors on the SCALES Leviathan Board. It will output the information as a struct as telemetry. The MCP Manager will also have an internal state machine that will parse parameters and output a state dependent on the current thermal readings of the system.

## Requirements

| **Name** | **Description** | **Validation** |
| --- | --- | --- |
| MCPM-001 | Collect thermal readings as a ThermalReading defined type periodically and output it as telemetry | Unit Test/Inspection |
| MCPM-002 | Output STATES dependent on configurable thermal parameter zones to the Spacecraft State Manager | Unit Test/Inspection |

## Design

### Diagrams

!image.png

### Typical Usage

End user will check temperature parameters in real time. If thermal zones increase or decrease, states will be outputted to notify SpaceCraftStateManager of system health.

## Port Descriptions

| **Kind** | **Name** | **Type** | **Description** |
| --- | --- | --- | --- |
| output port | mcpWriteRead | Drv.I2cWriteRead | Create output port to use the Drv.I2cWriteRead type to bring in i2cwriteread functionality to talk to sensors |
| async input port | pollTempData | Svc.Sched | Port to be invoked by scheduler, causes component to print collected data |

## Component States

| **Name** | **Description** |
| --- | --- |
| IDLE | Idle state for idle temperature ranges |
| WARN | Warn state for warn temperature ranges |
| FATAL | Fatal state for fatal temperature ranges |

## Parameters

| **Name** | **Description** |
| --- | --- |
| MCP_IDLE_LOW | F32 (id 0x00) Idle zone low temperature threshold. |
| MCP_IDLE_HIGH | F32 (id 0x01) Idle zone high temperature threshold. |
| MCP_WARN_LOW | F32 (id 0x02) Warning zone low temperature threshold. |
| MCP_WARN_HIGH | F32 (id 0x03) Warning zone high temperature threshold. |
| MCP_FAULT_LOW | F32 (id 0x04) Fault zone low temperature threshold. |
| MCP_FAULT_HIGH | F32 (id 0x05) Fault zone high temperature threshold. |

## Telemetry

| **Name** | **Description** |
| --- | --- |
| MCP_IDLE_LOW | F32 (id 0x10) Idle zone low temperature threshold. |
| MCP_IDLE_HIGH | F32 (id 0x11) Idle zone high temperature threshold. |
| MCP_WARN_LOW | F32 (id 0x12) Warning zone low temperature threshold. |
| MCP_WARN_HIGH | F32 (id 0x13) Warning zone high temperature threshold. |
| MCP_FAULT_LOW | F32 (id 0x14) Fault zone low temperature threshold. |
| MCP_FAULT_HIGH | F32 (id 0x15) Fault zone high temperature threshold. |

## Unit Tests

| **Name** | **Description** | **Output** | **Coverage** |
| --- | --- | --- | --- |
| MCPMUT-001 | Verify that I2cWriteRead cycle is complete, then prints data over 2 cycles | Correct | 100% |
| MCPMUT-002 | Verify that Temperature parameters output expected STATES for each sensor  | Works in GDS and a cycle in unit tests | 70% |

## Change Log

| **Name** | **Description** | **Authors** |
| --- | --- | --- |
| 1.0.0 | Initial component implementation without state machine, parameters, or port to SSM. Just telemetry outputting | Luca Lanzillotta, Dat Nguyen |
| 1.1.9 | Added Unit tests, parameters, and State output | Dat Nguyen |