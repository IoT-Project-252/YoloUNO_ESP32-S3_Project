# Internet of Things Project Assignment

**Deadline:** 06/12/2025  
**Group members:** 2-3 students per group  
**Submission:** PDF report and GitHub link

## 1. Project Overview

In this project, students will gain hands-on experience with real-time embedded systems by modifying and extending an existing RTOS-based project. You will work with the project YoloUNO_PlatformIO - RTOS_Project, which is implemented on a microcontroller development board. The mission is to develop a new, complete project with functionalities that differ by at least 30% from the original code base. The board used in this project will be setup at 301B9 and 812H6. Some manual videos can be found at https://www.youtube.com/watch?v=l7AqpsF1ByQ&list=PLyD_mbw_VznMG_tZ-KAGOh6hRPceHuIl5

## 2. Project Tasks and Requirements

Basically, students can propose additional features of their own, but these must be of a scope and complexity equivalent to the five suggested tasks bellow. All programming and development work must be carried out on the ESP32 S3 platform using the PlatformIO plug-in.

Your group must implement, document, and demonstrate the following tasks, adapting and extending them with new features and logic as outlined below:

### Task 1: Single LED Blink with Temperature Conditions

- Redefine the LED blinking behavior to respond to different temperature conditions (at least 3 different behaviors).
- Use semaphores to manage task synchronization in your implementation.
- Ensure that the condition handling and semaphore logic are clearly explained in your code and report.

### Task 2: NeoPixel LED Control Based on Humidity

- Redefine NeoPixel (RGB LED) color patterns to represent different humidity levels (at least 3 levels/colors).
- Utilize semaphore synchronization technique for updating and displaying color changes.
- Clearly show the mapping between humidity value ranges and colors.

### Task 3: Temperature and Humidity Monitoring with LCD Display

- Define conditions for creating/releasing semaphores based on sensor readings.
- At least 3 different display states (e.g., normal, warning, critical) according to measurement conditions are raised (by using semaphore)
- Remove ALL global variables in your project (by using semaphore in RTOS)

### Task 4: Web Server in Access Point Mode

- Redesign the web server interface for better usability.
- Web server must enable control over two devices (e.g., LED1 and LED2, which can be renamed or replaced as appropriate for your group's application).
- The interface must include at least two buttons and appropriately labeled control actions.

### Task 5: TinyML Deployment & Accuracy Evaluation

- Describe the dataset used for model training, including data collection and labeling steps.
- Implement a TinyML model and run it on your microcontroller.
- Measure and evaluate the recognition accuracy of your model on the hardware, and provide discussion and conclusion regarding performance.

### Task 6: Data Publishing to CoreIOT Cloud Server

- Implement the functionality to publish sensor data (such as temperature and humidity) from the ESP32 S3 to the CoreIOT server (https://app.coreiot.io/).
- **Important:** The ESP32 S3 device must be in Station (STA) mode (connected to the Internet via WiFi) in order to successfully publish data to the cloud server.
- You must configure and use the appropriate authentication token that corresponds to your registered Device on the CoreIOT server. Ensure that the device token in your code matches the one provided for your specific device within the CoreIOT platform.
- Please use a solution template from CoreIOT for your project

## 3. Submission Requirements

### Report
PDF file detailing the design, implementation, and results of all five tasks. The report must include:
- Introduction and Objectives
- Description of functionalities implemented in all 6 tasks
- Implementation highlights (e.g., semaphore logic, web server redesign, semaphore, TinyML workflow, CoreIOT workflow)
- Evaluation/Experimental results (especially for TinyML)
- Group discussion and conclusions
- Names and roles of all group members
- GitHub repository link to your group's source code

### Code
- Well-commented, original code. Your codebase must be hosted on GitHub and referenced in your report.

**Deadline:** Submit your PDF report via [Insert Submission Portal/Method] by 27/11/2025.

## 4. Assessment Rubric

Bonus points will be awarded for novel features, creative user interface, or advanced TinyML applications.

## 5. Regulations

- Each group must have 2-3 members.
- All group members must contribute equally; tasks and contributions should be stated explicitly in the report.
- Plagiarism or direct copying from other groups or sources is strictly prohibited and will result in disqualification.

## 6. Support and Resources

- Original project resources: https://github.com/nhanksd85/YoloUNO_PlatformIO/tree/RTOS_Project
- For technical issues contact in the Zalo group of the course

## 7. Timeline

- **Project Q&A:** Discuss in Zalo Group
- **Project submission:** 15/04/2026
