// University of Southampton, 2012
// EMECS Group Design Project

// Structs definitions for messages that will be exchanged between the
// monitoring device and the base station are stored here
// This is to ensure message content is consistent across both devices
// while serializing and deserializing raw data

#ifndef __MESSAGETYPES_H
#define __MESSAGETYPES_H

#include <stdint.h>
#include <stdbool.h>

// IAR CC uses a different keyword for packed structures
#if defined ( __ICCARM__ )
  #define PACKEDSTRUCT __packed struct
#else
  #define PACKEDSTRUCT struct __attribute__((__packed__))
#endif

enum MessageType {
	msgSensorData,
	msgSensorConfig,
	msgDebug
};

enum ConfigMsgType {
	typeSensorConfig,
	typeNodeConfig
};

enum DebugMsgType {
	typeDebugString
};

enum SensorType {
	typeGPS,
	typeAccelerometer,
	typeTemperature,
	typeRawTemperature,
	typeHeartRate
};



// definition of the data structure at the highest abstraction level.
// This structure is used to transmit data between monitoring
// devices and the base station
typedef PACKEDSTRUCT {
	enum MessageType mainType;
	uint16_t payloadLength;
	uint8_t *payload;
} MessagePacket;

// definition of the three main message groups: Sensor, Config and Debug
// They contain a subtype field for specialization
typedef PACKEDSTRUCT {
	enum SensorType;
	uint32_t startTimestampMs;
	uint16_t sampleIntervalMs;
	uint8_t arrayLength;	// could also be calculated during de-serialization
	uint8_t *sensorMsgArray;
} SensorMessage;

typedef PACKEDSTRUCT {
	enum ConfigMsgType;
	uint8_t arrayLength;
	uint8_t *configMsgArray;
} ConfigMessage;

typedef PACKEDSTRUCT {
	enum DebugMsgType;
	uint8_t *debugData;
} DebugMessage;

// definition of subtypes for ConfigMessages
typedef PACKEDSTRUCT {
	enum SensorType;
	uint8_t enableSensor;
	uint16_t sampleIntervalMs;
	uint16_t samplePeriodMs;
} ConfigSensor;

typedef PACKEDSTRUCT {
	uint8_t *data;
} ConfigNode;

// definition of subtypes for SensorMessage
typedef PACKEDSTRUCT {
	uint8_t degree;
	uint8_t minute;
	uint8_t second;
} Coordinate;

typedef PACKEDSTRUCT {
	Coordinate latitude;
	bool latitudeNorth;
	Coordinate longitude;
	bool longitudeWest;
	bool validPosFix;
} GPSMessage;

typedef PACKEDSTRUCT {
	int16_t x;
	int16_t y;
	int16_t z;
} AccelerometerMessage;

typedef PACKEDSTRUCT {
	double Tobj;
} TemperatureMessage;

typedef PACKEDSTRUCT {
	double Vobj;
	double Tenv;
} RawTemperatureMessage;

typedef PACKEDSTRUCT {
	uint8_t bpm;
} HeartRateMessage;

#endif	// __MESSAGETYPES_H
