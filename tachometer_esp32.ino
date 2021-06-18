#include <vector>
#include <SPI.h>
#include "kiss_fftr.h"

#define HSPI_SS      15
#define SPI_CLK      1000000

// Endereços dos registradores do acelerômetro ADXL345
#define DEVICE_ID    0x00
#define BW_RATE      0x2C // Rate bits configurados como 1010 (100Hz) por padrão
#define POWER_CTL    0x2D
#define DATA_FORMAT  0x31
#define DATAX0       0x32
#define DATAX1       0x33
#define DATAY0       0x34
#define DATAY1       0x35
#define DATAZ0       0x36
#define DATAZ1       0x37
#define FIFO_CTL     0x38

#define SAMPLE_RATE      32
#define SAMPLE_PERIOD_US 31250
#define SIGNAL_LENGTH    256
#define THRESHOLD        0.2f
#define ACCEL_RATIO      (4.0f / 1024)

void setup_accel(uint cs_pin, SPIClass *adxl345) {
    pinMode(cs_pin, OUTPUT);
    digitalWrite(cs_pin, HIGH);

    uint8_t values[] = { FIFO_CTL, 0x8F };
    adxl345->beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE3));
    digitalWrite(cs_pin, LOW);
    adxl345->transfer(values, 2);
    digitalWrite(cs_pin, HIGH);
    adxl345->endTransaction();

    // O ADXL345 inicia em modo de espera. 0x08 (00001000) inicia amostragem
    values[0] = POWER_CTL;
    values[1] = 0x08;
    adxl345->beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE3));
    digitalWrite(cs_pin, LOW);
    adxl345->transfer(values, 2);
    digitalWrite(cs_pin, HIGH);
    adxl345->endTransaction();
}

void read_accelerometer(uint cs_pin, SPIClass *adxl345, int16_t *x, int16_t *y, int16_t *z) {
    uint8_t buffer[6] = { 0 };
    uint8_t reg = DATAX0;
    reg |= 0xC0; // Modo de leitura, múltiplos bytes

    adxl345->beginTransaction(SPISettings(SPI_CLK, MSBFIRST, SPI_MODE3));
    digitalWrite(cs_pin, LOW);
    adxl345->transfer(&reg, 1);
    adxl345->transferBytes(nullptr, buffer, 6);
    digitalWrite(cs_pin, HIGH);
    adxl345->endTransaction();

    *x = buffer[0] | (buffer[1] << 8);   
    *y = buffer[2] | (buffer[3] << 8);   
    *z = buffer[4] | (buffer[5] << 8);
}

std::vector<float> samples;
kiss_fftr_cfg fftcfg;
kiss_fft_cpx freqData[SIGNAL_LENGTH];
float currentRPM;

SPIClass adxl345;
unsigned long sampleTimestamp, printTimestamp;
void setup() {
    Serial.begin(115200);

    // Configura a acelerômetro
    adxl345.begin();
    setup_accel(HSPI_SS, &adxl345);

    // Inicializa o buffer de saída para cálculo da FFT
    fftcfg = kiss_fftr_alloc(SIGNAL_LENGTH, false, 0, 0);

    sampleTimestamp = micros();
    printTimestamp = sampleTimestamp;
}

void loop() {
    unsigned long now = micros();
    if (now - sampleTimestamp >= SAMPLE_PERIOD_US) {
        sampleTimestamp = now;

        int16_t x, y, z;
        read_accelerometer(HSPI_SS, &adxl345, &x, &y, &z);
        samples.push_back(x * ACCEL_RATIO);
        if (samples.size() > SIGNAL_LENGTH) // Remove amostra mais antiga
            samples.erase(samples.begin());
        
        // Calcula a FFT
        kiss_fftr(fftcfg, samples.data(), freqData);

        // Encontra a maior componente de frequência
        int biggestIndex = 0;
        float maxValue = freqData[0].r * freqData[0].r + freqData[0].i * freqData[0].i;
        int i;
        for (i = 1; i < SIGNAL_LENGTH / 2; ++i) {
            float mag = freqData[i].r * freqData[i].r + freqData[i].i * freqData[i].i;
            // Descarta ruídos
            if (mag >= THRESHOLD && mag > maxValue) {
                maxValue = mag;
                biggestIndex = i;
            }
        }
        // Converte para RPM
        currentRPM = biggestIndex * 60.0f * SAMPLE_RATE / SIGNAL_LENGTH;
    }

    if (now - printTimestamp > 1000000) {
        printTimestamp = now;
        Serial.print("RPM: ");
        Serial.print(currentRPM);
        Serial.println("");
    }
}
