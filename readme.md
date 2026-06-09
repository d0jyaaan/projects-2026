# STM32 RFID Access Control System

A password-protected RFID access control system built with the **STM32F446RE**. The project combines RFID authentication, keypad password verification, OLED display feedback, LEDs and a buzzer alarm to create a simple two-factor security system.

---
## Picture
Unpowered
<img width="1280" height="913" alt="image" src="https://github.com/user-attachments/assets/18f8d779-1cab-487f-8311-93c70467f2ff" />
Powered
<img width="1280" height="936" alt="image" src="https://github.com/user-attachments/assets/9d3695db-cafe-425c-876a-9e21c5061b7e" />

---
## Demo Video

> *(https://youtu.be/hUpy1Zz4KwQ?si=nej2KByEA1a0SdGY)*

[![Watch Demo](https://img.shields.io/badge/YouTube-Watch%20Demo-red)](YOUR_YOUTUBE_LINK)

---
## Schematic Diagram

<img width="1497" height="891" alt="image" src="https://github.com/user-attachments/assets/6a6bd3e0-1bb1-41a1-b26a-ece12fe04452" />

## Features

* RFID card authentication
* 4-digit user password verification
* Master password protection
* Register and delete RFID cards
* Reset user and master passwords
* Full system reset
* OLED display interface
* Green / Yellow / Red LED status indicators
* Audible buzzer feedback
* Alarm mode after three incorrect password attempts
* Alarm disarming using master password
* Support for up to **100 RFID tags**

---

## Hardware Used

| Component            | Description          |
| -------------------- | -------------------- |
| STM32F446RE          | Main microcontroller |
| RC522 RFID Module    | RFID reader          |
| 4×4 Matrix Keypad    | Password input       |
| SSD1306 OLED Display | User interface       |
| Passive Buzzer       | Audio feedback       |
| LEDs                 | Status indicators    |
| Power Supply         | 3.3V / 5V            |

---

## Authentication Process

```text
RFID Scan
     ↓
Is card registered?
     ↓
     Yes
     ↓
Enter User Password 
     ↓
Correct Password? → No → Reprompt password up to 3 times  → Incorrect password
     ↓                             ↓                                ↓
     Yes                    Correct password               Trigger alarm system
     ↓                             ↓
Access Granted               Access Granted
```

Three consecutive incorrect password attempts trigger the alarm mode.

---

## Functionalities

### A — Register RFID Tag

* Requires master password.
* Scans a new RFID card.
* Prevents duplicate registrations.
* Confirmation required before storing.

### B — Delete RFID Tag

* Requires master password.
* Scans an existing card.
* Confirmation required before deletion.

### C — Reset Password

* Requires master password.
* Allows changing:

  * Master password
  * User password

### D — Reset System

* Requires master password.
* Clears all registered RFID tags.
* Resets passwords.
* Returns system to startup configuration.

---

## Alarm Mode

The system enters alarm mode when:

* User password is entered incorrectly three times.

During alarm mode:

* Red LED flashes.
* Buzzer continuously sounds.
* Master password is required to disarm the system.

---

## System Components

* STM32F446RE
* RC522 RFID Reader
* SSD1306 OLED Display
* 4×4 Keypad
* Passive Buzzer
* Status LEDs

---

## Libraries Used

* STM32 HAL
* RC522 RFID Library
* SSD1306 OLED Library
