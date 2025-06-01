# ELIFANT-Event Manual

## Overview

**ELIFANT-Event** is a software system designed to manage, process, and analyze a data acquired by DELILA. It provides events respect to a user define trigger detectors and logics. 

## Getting Started

1. **Requirement**  
    The software itself can be downloaded from the official repository. [GitHub](https://github.com/aogaki/ELIFANT-Event). CMake is used for building the software. ROOT is required as a dependency. Follow the instructions in the repository to set up the environment. To find files, this uses std::filesystem, which is available in C++17 and later. The version of CMake used should be 3.5 or later. git is also required to clone the repository and manage updates.

2. **Example use case**
2.1. Download the ELIFANT-Event software.
``` bash
git clone https://github.com/aogaki/ELIFANT-Event.git
cd ELIFANT-Event
```
It is recommended to create a build directory named after the run number (e.g., `run123`) and run CMake from within that directory.
``` bash
mkdir run123
cd run123
cmake ..
make
```

2. Start initializing setting files.
```bash
./eve-builder -i
```
ELIFANT-Event ask you to input several information bellow:
```bash
Initializing the event builder...
Please enter the following information:
What is the data directory? (default: /Users/aogaki/WorkSpace/ELIFANT2025/3He12C/data/): 
What is the run number? (default: 76): 123
What is the start version? (default: 0): 
What is the end version? (default: 173): 9999
What is the time window? (default: 300): 1000
What is the coincidence window? (default: 300): 100
How many modules? (default: 11): 
How many channels of module 0? (default: 32): 16
How many channels of module 1? (default: 16): 16
How many channels of module 2? (default: 16): 
How many channels of module 3? (default: 16): 
How many channels of module 4? (default: 16): 
How many channels of module 5? (default: 16): 
How many channels of module 6? (default: 16): 
How many channels of module 7? (default: 16): 
How many channels of module 8? (default: 16): 32
How many channels of module 9? (default: 32): 
How many channels of module 10? (default: 32): 16
What is the time reference module? (default: 9): 
What is the time reference channel? (default: 0): 
What is the channel settings file name? (default: chSettings.json): 
What is the channel settings file name? (default: L2Settings.json): 
Generating settings template...
settings.json generated.
chSettings.json generated.
Initialization completed.
```
The time window is used for the time allignment of the data. The unit of it is ns. 1000 ns is enough for 11 digitizers internal delay. The coincidence window is used for the time coincidence of the data. The unit of it is ns. 100 ns is enough for Si, PMT, and Ge detectors. But sometimes, Ge detector makes more than 100 ns delay. If you can not find the correlation, make it bigger (e.g. 500 ns). The channel settings file name is used to set the channel settings. The structure will be described later. L2Settings.json is used to set the L2 trigger, event selection, settings. The structure will be described later.

### chSettings.json
```json
        {
            "ACChannel": 8,
            "ACModule": 4,
            "Channel": 4,
            "DetectorType": "PMT",
            "Distance": 0.0,
            "HasAC": true,
            "ID": 36,
            "IsEventTrigger": true,
            "Module": 9,
            "Phi": 0.0,
            "Tags": [
                "PMT"
            ],
            "Theta": 0.0,
            "ThresholdADC": 0,
            "p0": 14.8652,
            "p1": 0.717344,
            "p2": 0.0,
            "p3": 0.0,
            "x": 0.0,
            "y": 0.0,
            "z": 0.0
        },
```


3. Dispatch the `UserRegistered` event upon successful user registration.
4. The handler processes the event and sends the notification.
5. All actions are logged for auditing purposes.

## API Reference

- **register_event(event_type, handler):** Registers a new event type and its handler.
- **dispatch_event(event_type, payload):** Dispatches an event with the specified payload.
- **add_notification_channel(channel):** Adds a new notification channel (e.g., email, SMS).
- **get_event_log():** Retrieves the event log for auditing and analysis.

## Support

For further assistance, refer to the official documentation or contact the support team.

---