// Network and Authentication credentials
// Replace these with your actual credentials before uploading to your device
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define BLYNK_TEMPLATE_ID "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "Air Monitoring"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <DHT.h>
#include <math.h>
#include <BlynkSimpleEsp32.h>

// Pin Definitions - changed to const for better type safety
const uint8_t DHTPIN = 33;
const uint8_t MQ135PIN = 34;
const uint8_t MQ7PIN = 35;
#define DHTTYPE DHT22

// Sensor calibration constants - using const for better type safety
const float RLOAD = 10.0;
const float MQ135_RZERO = 76.63;
const float MQ7_RZERO = 5.0;
const float MQ135_SCALE = 100.0;
const float MQ7_SCALE = 5.0;
const float MQ135_SO2_SCALE = 0.5;

// Timing constants
const unsigned long readingInterval = 30000;  // 30 seconds between readings
const unsigned long sensorWarmupTime = 30000; // 30 seconds warmup time

/**
 * GasSensor Class
 * Handles gas sensor readings and calculations
 */
class GasSensor {
private:
    const uint8_t pin;
    const float rzero;
    const float rload;
    
public:
    GasSensor(uint8_t _pin, float _rzero, float _rload) : 
        pin(_pin), rzero(_rzero), rload(_rload) {}
    
    float readRatio() const {
        int adc = analogRead(pin);
        float voltage = (float)adc * (3.3f / 4095.0f);
        float rs = ((3.3f * rload) / voltage) - rload;
        return rs / rzero;
    }
};

// AQI Calculation struct
struct AQIResult {
    int aqi;
    String category;
};

// Initialize sensors
DHT dht(DHTPIN, DHTTYPE);
GasSensor mq135(MQ135PIN, MQ135_RZERO, RLOAD);
GasSensor mq7(MQ7PIN, MQ7_RZERO, RLOAD);

// Global variables
unsigned long lastReadingTime = 0;

// Function prototypes
AQIResult calculateAQI(float co2_ppm, float co_ppm, float so2_ppm);
inline float calculateCO2(float ratio, float temp, float humidity);
inline float calculateCO(float ratio, float temp, float humidity);
inline float calculateSO2(float ratio, float temp, float humidity);

/**
 * Calculate Air Quality Index based on multiple gas concentrations
 */
AQIResult calculateAQI(float co2_ppm, float co_ppm, float so2_ppm) {
    AQIResult result;
    int aqi_co2 = 0;
    int aqi_co = 0;
    int aqi_so2 = 0;
    
    // CO2 AQI calculation (custom scale as EPA doesn't include CO2)
    if (co2_ppm <= 700) {
        aqi_co2 = map(co2_ppm, 400, 700, 0, 50);
    } else if (co2_ppm <= 1000) {
        aqi_co2 = map(co2_ppm, 701, 1000, 51, 100);
    } else if (co2_ppm <= 2000) {
        aqi_co2 = map(co2_ppm, 1001, 2000, 101, 150);
    } else if (co2_ppm <= 5000) {
        aqi_co2 = map(co2_ppm, 2001, 5000, 151, 200);
    } else {
        aqi_co2 = 301;
    }

    // CO AQI calculation (based on EPA standards)
    if (co_ppm <= 4.4) {
        aqi_co = map(co_ppm * 10, 0, 44, 0, 50);
    } else if (co_ppm <= 9.4) {
        aqi_co = map(co_ppm * 10, 45, 94, 51, 100);
    } else if (co_ppm <= 12.4) {
        aqi_co = map(co_ppm * 10, 95, 124, 101, 150);
    } else if (co_ppm <= 15.4) {
        aqi_co = map(co_ppm * 10, 125, 154, 151, 200);
    } else {
        aqi_co = 301;
    }

    // SO2 AQI calculation (custom scale based on sensor data)
    if (so2_ppm <= 0.5) {
        aqi_so2 = map(so2_ppm, 0, 0.5, 0, 50);
    } else if (so2_ppm <= 1.0) {
        aqi_so2 = map(so2_ppm, 0.51, 1.0, 51, 100);
    } else if (so2_ppm <= 5.0) {
        aqi_so2 = map(so2_ppm, 1.01, 5.0, 101, 150);
    } else if (so2_ppm <= 10.0) {
        aqi_so2 = map(so2_ppm, 5.01, 10.0, 151, 200);
    } else {
        aqi_so2 = 301;
    }

    // Take the worst of the three readings
    result.aqi = max(aqi_co2, max(aqi_co, aqi_so2));

    // Determine AQI category
    if (result.aqi <= 50) {
        result.category = "Good";
    } else if (result.aqi <= 100) {
        result.category = "Moderate";
    } else if (result.aqi <= 150) {
        result.category = "Unhealthy for Sensitive Groups";
    } else if (result.aqi <= 200) {
        result.category = "Unhealthy";
    } else if (result.aqi <= 300) {
        result.category = "Very Unhealthy";
    } else {
        result.category = "Hazardous";
    }

    return result;
}

/**
 * Calculate CO2 concentration from MQ135 sensor
 */
inline float calculateCO2(float ratio, float temp, float humidity) {
    float correction = 1.0f + 0.015f * (temp - 20.0f) + 0.008f * (humidity - 65.0f);
    float compensatedRatio = ratio * correction;
    return 400.0f + (MQ135_SCALE * (1.0f - compensatedRatio));
}

/**
 * Calculate CO concentration from MQ7 sensor
 */
inline float calculateCO(float ratio, float temp, float humidity) {
    // Use correction for temperature and humidity
    float correction = 1.0f + 0.015f * (temp - 20.0f) + 0.008f * (humidity - 65.0f);
    
    // Adjusted formula based on calibration for CO (MQ7)
    float co_ppm = MQ7_SCALE * (1.0f - ratio) * correction;
    
    // Apply a more realistic boundary condition for CO levels
    return constrain(co_ppm, 0.0f, 50.0f); // Typical indoor air quality range
}

/**
 * Calculate SO2 concentration from MQ135 sensor
 */
inline float calculateSO2(float ratio, float temp, float humidity) {
    // Apply correction factors for temperature and humidity
    float correction = 1.0f + 0.015f * (temp - 20.0f) + 0.008f * (humidity - 65.0f);
    
    // Adjusted formula for SO2 based on experimental data
    float so2_ppm = (0.1f + (MQ135_SO2_SCALE * (1.0f - ratio))) * correction;
    
    // Apply boundary conditions based on typical levels for SO2
    return constrain(so2_ppm, 0.0f, 10.0f);
}

void setup() {
    Serial.begin(115200);
    dht.begin();
    
    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(300);
    }
    Serial.println("\nConnected to WiFi");

    // Initialize Blynk
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);

    Serial.println("Warming up sensors... (30 seconds)");
    delay(sensorWarmupTime);
}

void loop() {
    Blynk.run();  // Run Blynk

    // Check if it's time to read sensors and send data
    unsigned long currentTime = millis();
    if (currentTime - lastReadingTime > readingInterval || lastReadingTime == 0) {
        lastReadingTime = currentTime;
        
        // Read temperature and humidity
        float temperature = dht.readTemperature();
        float humidity = dht.readHumidity();

        if (isnan(temperature) || isnan(humidity)) {
            Serial.println(F("Failed to read from DHT sensor!"));
            return;
        }

        // Read gas sensors - read once and store
        float mq135_ratio = mq135.readRatio();
        float mq7_ratio = mq7.readRatio();
        
        // Calculate gas concentrations
        float co2_ppm = constrain(calculateCO2(mq135_ratio, temperature, humidity), 400.0f, 5000.0f);
        float co_ppm = constrain(calculateCO(mq7_ratio, temperature, humidity), 0.0f, 100.0f);
        float so2_ppm = constrain(calculateSO2(mq135_ratio, temperature, humidity), 0.0f, 10.0f);

        // Calculate Air Quality Index
        AQIResult aqi = calculateAQI(co2_ppm, co_ppm, so2_ppm);

        // Display results on Serial Monitor
        Serial.println(F("=== Sensor Data ==="));
        Serial.print(F("Temperature: "));
        Serial.print(temperature);
        Serial.println(F("°C"));
        Serial.print(F("Humidity: "));
        Serial.print(humidity);
        Serial.println(F("%"));
        Serial.print(F("CO2: "));
        Serial.print(co2_ppm);
        Serial.println(F(" ppm"));
        Serial.print(F("CO: "));
        Serial.print(co_ppm);
        Serial.println(F(" ppm"));
        Serial.print(F("SO2: "));
        Serial.print(so2_ppm);
        Serial.println(F(" ppm"));
        Serial.print(F("AQI: "));
        Serial.print(aqi.aqi);
        Serial.println(F(" - ") + aqi.category);
        Serial.println(F("==================="));

        // Send data to Blynk
        Blynk.virtualWrite(V0, co2_ppm);  // CO2 data
        Blynk.virtualWrite(V1, co_ppm);   // CO data
        Blynk.virtualWrite(V2, so2_ppm);  // SO2 data
        Blynk.virtualWrite(V3, aqi.aqi);  // AQI value
        Blynk.virtualWrite(V4, aqi.category.c_str());  // AQI category
        Blynk.virtualWrite(V5, temperature);  // Temperature
        Blynk.virtualWrite(V6, humidity);     // Humidity
    }
}

        delay(1000);
    }
}