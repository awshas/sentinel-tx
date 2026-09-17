# Sentinel-tx requirments

This is a requirments document that documents the intended behaviour and measurment quality of the **Sentinel-tx**.

There are (for now) 4 types of requirements:
1. **Functionality**
2. **Measurment quality**
3. **Communication**
4. **Fault handling**

The template of writing a requirment is:

    <REQ-Tnn>: <Status>
        The <system> <modal verb> <response>
        Test: <Test setup to test REQ-Tnn>

In **REQ_Tnn**:
- T: The type of the requirment
- nn: the number of the requirment

For **Status** there are 3 possible values:
- **Not implemented**: This requirment is not yet implemented.
- **Not Tested**: The feature is implemented but not yet tested against the requirment.
- **Done**: This requirment is implemented and tested against the requirment.

The requirment numbers don't get reused, even if for example REQ_101 is done there should be no new REQ_101.

An example of a requirment:

    REQ_201: 

        The Sentinel-tx shall measure temperature with ±0.2 C accuracy.

        Test: To test this, the measurments of a temperature measurment tool with ±0.2 C accuracy or better 
        shall be compared to the Sentinel-tx measurments.





## Requirments

### Functionality

#### REQ_101:

    When the watchdog is triggered, the Sentinel-tx shall restart.
    
    Test: Instead of a timer, the watchdog petting mechanisim will be triggered by a button press. That way the petting will be simulated and can be stopped when the tester stops pushing the button.

### Measurment quality

#### REQ_201:

    The Sentinel-tx shall measure temperature with ±0.2 C accuracy.

    Test: To test this, the measurments of a temperature measurment tool with ±0.2 C accuracy or better 
    shall be compared to the Sentinel-tx measurments.

R

### Communication

### Fault handling