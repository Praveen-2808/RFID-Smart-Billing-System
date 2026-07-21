<img width="1774" height="887" alt="ChatGPT Image Jul 8, 2026, 07_48_01 PM" src="https://github.com/user-attachments/assets/a31960b7-917e-4a70-ad76-443c252edd0b" />

## Overview

The RFID Smart Billing System is an embedded systems project developed using the LPC2148 (ARM7) microcontroller. The system automates product billing by identifying RFID-tagged products, displaying product information on an LCD, communicating with a Linux PC over UART, retrieving product information from a database, and generating the final bill.

This project demonstrates practical implementation of embedded firmware, serial communication, hardware interfacing, and Linux integration.

---

## Features

- RFID-based product identification
- Automatic billing process
- LCD display for product details
- UART communication with Linux PC
- Linux-based product database
- Real-time bill generation
- Faster and error-free billing

---

## Technologies Used

| Hardware | Software |
|----------|----------|
| LPC2148 (ARM7) | Embedded C |
| RFID Reader | Linux |
| 16x2 LCD | GCC |
| UART | Serial Communication |
| Power Supply | Makefile |

---

## System Architecture

<img width="1774" height="887" alt="ChatGPT Image Jul 8, 2026, 07_48_28 PM" src="https://github.com/user-attachments/assets/bd93ccbf-bd51-4802-b03f-267f66bade96" />


The RFID reader scans the product tag and sends the Tag ID to the LPC2148. The microcontroller processes the request, communicates with the Linux database using UART, receives the product information, displays it on the LCD, and updates the billing data.

---

## Project Workflow

<img width="1774" height="887" alt="ChatGPT Image Jul 8, 2026, 07_48_37 PM" src="https://github.com/user-attachments/assets/90679bed-837b-4f8c-b738-7cf4af4428e5" />

### Workflow
<img width="1536" height="1024" alt="ChatGPT Image Jul 8, 2026, 08_55_48 PM" src="https://github.com/user-attachments/assets/5f3066e7-e1f8-4734-b92c-e43e18290a94" />


1. Scan RFID Tag
2. Read Tag ID
3. Identify Product
4. Display Product Details
5. Update Linux Database
6. Generate Bill

---

## Hardware Block Diagram

<img width="1536" height="1024" alt="ChatGPT Image Jul 8, 2026, 07_48_45 PM" src="https://github.com/user-attachments/assets/4c419bf4-5629-420b-bb5d-73d59738f682" />
<img width="1071" height="1428" alt="WhatsApp Image 2026-07-21 at 11 25 19 PM" src="https://github.com/user-attachments/assets/3d57de1c-293d-4be1-a052-20704a74a4b6" />


### Hardware Components

- LPC2148 ARM7 Microcontroller
- RFID Reader
- 16×2 LCD Display
- Linux PC
- UART Interface
- 5V Power Supply

---

## UART Communication

<img width="1536" height="1024" alt="ChatGPT Image Jul 8, 2026, 07_48_53 PM" src="https://github.com/user-attachments/assets/d39906f4-7c18-452f-af57-a00d6fd740d4" />

The LPC2148 exchanges product information with the Linux PC through UART communication.

Communication Sequence:

- RFID Tag Scanned
- LPC2148 reads Tag ID
- Tag ID sent to Linux
- Linux searches database
- Product details returned
- LCD updated
- Bill generated

## Hardware Required

- LPC2148 Development Board
- RFID Reader Module
- RFID Tags
- 16×2 LCD Display
- USB-UART Converter
- Linux PC
- Power Supply

---

## Software Required

- Keil uVision
- Flash Magic
- Ubuntu/Linux
- GCC Compiler
- Terminal Software (Minicom / GTKTerm)

---

## How to Run

### Embedded Side

1. Open the project in Keil.
2. Build the project.
3. Generate the HEX file.
4. Flash the LPC2148 using Flash Magic.

### Linux Side

1. Connect the UART cable.
2. Compile the Linux application.
3. Start the database program.
4. Scan RFID tags.
5. View billing information.

---

## Applications

- Smart Retail Stores
- Supermarkets
- Library Management
- Inventory Management
- Warehouse Automation
- Cashless Billing Systems

---

## Future Enhancements

- Barcode Scanner Support
- QR Code Billing
- Wi-Fi Connectivity
- Cloud Database
- Mobile Application
- Online Payment Integration
- Receipt Printing
- IoT Dashboard

---

## Author

**Jada Praveen**

Embedded Systems Engineer

GitHub:
https://github.com/Praveen-2808

---

## License

This project is intended for educational and learning purposes.
