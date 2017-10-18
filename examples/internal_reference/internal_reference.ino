/* Example for the use of the internal voltage reference VREF
*   VREF can vary between 1.173 V and 1.225 V, so it can be adjusted by software.
*   This example shows how to find the optimum value for this adjustment (trim).
*   You need a good multimeter to measure the real voltage difference between AGND and 3.3V,
*   change voltage_target accordingly and run the program. The optimum trim value is printed at the end.
*   The bisection algorithm prints some diagnostics while running.
*   Additionally, the bandgap voltage is printed too, it should lie between 0.97 V and 1.03 V, with 1.00 V being the typical value.
*   Because of noise, the trim value is not accurate to 1 step, it fluctuates +- 1 or 2 steps.
*/

#include <ADC.h>
#include <VREF.h>

ADC* adc = new ADC();

//! change this value to your real input value, measured between AGND and 3.3V
float voltage_target = 3.3;

//! change the TOL (tolerance, minimum difference  in mV between voltage and target that matters)
//! to refine even more, but not to less than 0.5 mV
const float TOL = 1.0;
// Maximum iterations of the algorithm, no need to change it.
const uint8_t MAX_ITER = 100;


// Get the voltage of VREF using the trim value
float get_voltage(uint8_t trim) {
    VREF::trim(trim);
    VREF::waitUntilStable();
    return 1.20/adc->analogRead(ADC_INTERNAL_SOURCE::VREF_OUT)*(adc->getMaxValue(ADC_0));
}

// Simple bisection method to get the optimum trim value
// This method is not the bests for noisy functions...
// Electrical datasheet: "The Voltage Reference (VREF) is intended to supply an accurate voltage output
// that can be trimmed in 0.5 mV steps."
int8_t optimumTrimValue(float voltage_target) {
    // https://en.wikipedia.org/wiki/Bisection_method#Algorithm
    // INPUT: Function f, endpoint values a, b, tolerance TOL, maximum iterations NMAX
    // CONDITIONS: a < b, either f(a) < 0 and f(b) > 0 or f(a) > 0 and f(b) < 0
    // OUTPUT: value which differs from a root of f(x)=0 by less than TOL
    //
    // N ← 1
    // While N ≤ NMAX # limit iterations to prevent infinite loop
    //   c ← (a + b)/2 # new midpoint
    //   If f(c) = 0 or (b – a)/2 < TOL then # solution found
    //     Output(c)
    //     Stop
    //   EndIf
    //   N ← N + 1 # increment step counter
    //   If sign(f(c)) = sign(f(a)) then a ← c else b ← c # new interval
    // EndWhile
    // Output("Method failed.") # max number of steps exceeded
    const uint8_t MAX_VREF_trim = 63;
    const uint8_t MIN_VREF_trim = 0;

    uint8_t niter = 0;
    uint8_t a = MIN_VREF_trim, b = MAX_VREF_trim, midpoint = (a + b)/2;
    float cur_diff, diff_a;

    // start VREF
    VREF::start(VREF_SC_MODE_LV_HIGHPOWERBUF, midpoint);

    Serial.println("niter,\ta,\tb,\tmidpoint,\tdiff (mV)");

    while (niter < MAX_ITER) {
        midpoint = (a + b)/2;
        cur_diff = (get_voltage(midpoint) - voltage_target)*1000;

        Serial.print(niter, DEC); Serial.print(",\t");
        Serial.print(a, DEC); Serial.print(",\t");
        Serial.print(b, DEC); Serial.print(",\t");
        Serial.print(midpoint, DEC); Serial.print(",\t\t");
        Serial.println(cur_diff, 2);

        if (abs(cur_diff) <= TOL || b-a < 1) {
            return midpoint;
        }
        niter++;
        diff_a = get_voltage(a) - voltage_target;
        if ((cur_diff < 0 && diff_a < 0) || (cur_diff > 0 && diff_a > 0)) {
            a = midpoint;
        }
        else {
            b = midpoint;
        }

    }
    return -1;
}

void setup() {
    Serial.begin(9600);

    // Best measurement conditions
    adc->setAveraging(32);
    adc->setResolution(16);
    adc->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);
    adc->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED);
    #if ADC_NUM_ADCS>1
    adc->setAveraging(32, ADC_1);
    adc->setResolution(16, ADC_1);
    adc->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED, ADC_1);
    adc->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_LOW_SPEED, ADC_1);
    #endif // ADC_NUM_ADCS

    delay(2000);

    uint8_t VREF_trim = optimumTrimValue(voltage_target);
    Serial.print("Optimal trim value: "); Serial.println(VREF_trim);

    VREF::start(VREF_SC_MODE_LV_HIGHPOWERBUF, VREF_trim);
    VREF::waitUntilStable();

    Serial.print("VREF value: ");
    Serial.print(1.20/adc->analogRead(ADC_INTERNAL_SOURCE::VREF_OUT)*adc->getMaxValue(ADC_0), 5);
    Serial.println(" V.");


    Serial.print("Bandgap value: ");
    Serial.print(3.3*adc->analogRead(ADC_INTERNAL_SOURCE::BANDGAP)/adc->getMaxValue(ADC_0), 5);
    Serial.println(" V.");

    VREF::stop();
}


void loop() {
}

/*

My output with a Teensy 3.6 connected to my laptop's USB.

With a cheap multimeter I measured voltage_target = 3.29 V, the results are:

niter,	a,	b,	midpoint,	diff (mV)
0,	    0,	63,	31,	    	38.62
1,  	31,	63,	47,	    	17.06
2,  	47,	63,	55,	    	6.81
3,  	55,	63,	59,	    	1.42
4,  	59,	63,	61,	    	-1.74
5,  	59,	61,	60,	    	-0.50
Optimal trim value: 60
VREF value: 3.28977 V.
Bandgap value: 1.00256 V.


Using voltage_target = 3.3, which should be true:

niter,	a,	b,	midpoint,	diff (mV)
0,	    0,	63,	31,	    	28.48
1,	    31,	63,	47,	    	7.20
2,  	47,	63,	55,	    	-3.33
3,	    47,	55,	51,	    	1.79
4,	    51,	55,	53,	    	-0.84
Optimal trim value: 53
VREF value: 3.29874 V.
Bandgap value: 1.00317 V.



My output with a Teensy 3.0 connected to my laptop's USB.
voltage_target = 3.22 V, the results are:

niter,	a,	b,	midpoint,	diff (mV)
0,  	0,	63,	31,		    37.34
1,  	31,	63,	47,	    	16.56
2,	    47,	63,	55,	    	5.54
3,	    55,	63,	59,	    	-0.00
Optimal trim value: 59
VREF value: 3.21947 V.
Bandgap value: 1.01978 V.


*/
