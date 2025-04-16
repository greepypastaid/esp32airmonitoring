#include <WiFi.h>
#include <DHT.h>
#include <math.h>

// Pin Definitions
#define DHTPIN 4
#define MQ135PIN 34
#define MQ7PIN 35
#define DHTTYPE DHT22

// Network credentials
#define WIFI_SSID "Enter Your SSID Here" // Replace with your SSID
#define WIFI_PASSWORD "Password Here" // Replace with your password

// Sensor calibration constants
#define RLOAD 10.0
#define MQ135_RZERO 76.63
#define MQ7_RZERO 5.0
#define MQ135_SCALE 100.0
#define MQ7_SCALE 5.0
#define MQ135_SO2_SCALE 0.5

// Timing constants
const unsigned long readingInterval = 30000;  // 30 seconds between readings
const unsigned long sensorWarmupTime = 30000; // 30 seconds warmup time

class GasSensor {
private:
    int pin;
    float rzero;
    float rload;
    
public:
    GasSensor(int _pin, float _rzero, float _rload) : 
        pin(_pin), rzero(_rzero), rload(_rload) {}
    
    float readRatio() {
        int adc = analogRead(pin);
        float voltage = (float)adc * (3.3 / 4095.0);
        float rs = ((3.3 * rload) / voltage) - rload;
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
int calculateSpecificAQI(float concentration, float minConcentration, float maxConcentration, int minAQI, int maxAQI);
String getCategory(int aqi);
float calculateCO2(float ratio, float temp, float humidity);
float calculateCO(float ratio, float temp, float humidity);
float calculateSO2(float ratio, float temp, float humidity);

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

    Serial.println("Warming up sensors... (30 seconds)");
    delay(sensorWarmupTime);
}

void loop() {
    if (millis() - lastReadingTime > readingInterval || lastReadingTime == 0) {
        lastReadingTime = millis();
        readAndDisplaySensorData();
    }
}

void readAndDisplaySensorData() {
    // Read temperature and humidity
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
    }

    // Read gas sensors
    float mq135_ratio = mq135.readRatio();
    float mq7_ratio = mq7.readRatio();
    
    // Calculate gas concentrations
    float co2_ppm = calculateCO2(mq135_ratio, temperature, humidity);
    float co_ppm = calculateCO(mq7_ratio, temperature, humidity);
    float so2_ppm = calculateSO2(mq135_ratio, temperature, humidity);

    // Constrain values to reasonable ranges
    co2_ppm = constrain(co2_ppm, 400, 5000);
    co_ppm = constrain(co_ppm, 0, 100);
    so2_ppm = constrain(so2_ppm, 0, 10);

    // Calculate Air Quality Index
    AQIResult aqi = calculateAQI(co2_ppm, co_ppm, so2_ppm);

    // Display results on Serial Monitor
    Serial.println("=== Sensor Data ===");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
    Serial.print("CO2: ");
    Serial.print(co2_ppm);
    Serial.println(" ppm");
    Serial.print("CO: ");
    Serial.print(co_ppm);
    Serial.println(" ppm");
    Serial.print("SO2: ");
    Serial.print(so2_ppm);
    Serial.println(" ppm");
    Serial.print("AQI: ");
    Serial.print(aqi.aqi);
    Serial.println(" - " + aqi.category);
    Serial.println("===================");

    delay(1000);
}

// Function to calculate AQI based on gas concentrations
AQIResult calculateAQI(float co2_ppm, float co_ppm, float so2_ppm) {
    AQIResult result;
    
    // CO2 AQI calculation (based on EPA-like scale for CO2)
    int aqi_co2 = calculateSpecificAQI(co2_ppm, 400, 5000, 0, 500);
    
    // CO AQI calculation (EPA standards)
    int aqi_co = calculateSpecificAQI(co_ppm, 0, 50, 0, 300);
    
    // SO2 AQI calculation (custom scale for SO2)
    int aqi_so2 = calculateSpecificAQI(so2_ppm, 0, 10, 0, 500);
    
    // Take the worst of the three readings
    result.aqi = max(aqi_co2, max(aqi_co, aqi_so2));

    // Determine AQI category
    result.category = getCategory(result.aqi);

    return result;
}

// Function to calculate specific AQI based on concentration
int calculateSpecificAQI(float concentration, float minConcentration, float maxConcentration, int minAQI, int maxAQI) {
    if (concentration <= minConcentration) return minAQI;
    if (concentration >= maxConcentration) return maxAQI;
    return map(concentration, minConcentration, maxConcentration, minAQI, maxAQI);
}

// Function to return AQI category based on AQI value
String getCategory(int aqi) {
    if (aqi <= 50) {
        return "Good";
    } else if (aqi <= 100) {
        return "Moderate";
    } else if (aqi <= 150) {
        return "Unhealthy for Sensitive Groups";
    } else if (aqi <= 200) {
        return "Unhealthy";
    } else if (aqi <= 300) {
        return "Very Unhealthy";
    } else {
        return "Hazardous";
    }
}

// Function to calculate CO2 concentration from MQ135 sensor
float calculateCO2(float ratio, float temp, float humidity) {
    float correction = 1.0 + 0.015 * (temp - 20) + 0.008 * (humidity - 65);
    float compensatedRatio = ratio * correction;
    return 400 + (MQ135_SCALE * (1.0 - compensatedRatio));
}

// Function to calculate CO concentration from MQ7 sensor
float calculateCO(float ratio, float temp, float humidity) {
    float correction = 1.0 + 0.015 * (temp - 20) + 0.008 * (humidity - 65);
    return MQ7_SCALE * (1.0 - ratio) * correction;
}

// Function to calculate SO2 concentration from MQ135 sensor
float calculateSO2(float ratio, float temp, float humidity) {
    float correction = 1.0 + 0.015 * (temp - 20) + 0.008 * (humidity - 65);
    return (0.1 + (MQ135_SO2_SCALE * (1.0 - ratio))) * correction;
}