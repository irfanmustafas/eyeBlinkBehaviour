/***
 *       Filename:  main.ino
 *
 *    Description:  Protocol for EyeBlinkConditioning.
 *
 *        Version:  0.0.1
 *        Created:  2017-04-11

 *         Author:  Dilawar Singh <dilawars@ncbs.res.in>
 *   Organization:  NCBS Bangalore
 *
 *        License:  GNU GPL2
 */

#include <avr/wdt.h>

#define         DRY_RUN                     1

// Pins etc.
#define         TONE_PIN                    2
#define         LED_PIN                     3
#define         MOTION1_PIN                 4
#define         MOTION2_PIN                 7
#define         CAMERA_TTL_PIN              10
#define         PUFF_PIN                    11
#define         SENSOR_PIN                  A5
#define         IMAGING_TRIGGER_PIN         13

/*-----------------------------------------------------------------------------
 *  Change parameters here.
 *-----------------------------------------------------------------------------*/
#define         TONE_FREQ                   4500
#define         TONE_DURATION                 50

#define         PUFF_DURATION                 50
#define         TRIAL_DURATION              2250


unsigned long stamp_ = 0;
unsigned dt_ = 2;
unsigned write_dt_ = 2;
unsigned trial_count_ = 0;
unsigned motion1 = 0;
unsigned motion2 = 0;
unsigned camera = 0;
unsigned microscope = 0;

char msg_[80];

unsigned long trial_start_time_ = 0;
unsigned long trial_end_time_ = 0;


char trial_state_[5] = "PRE_";

/*-----------------------------------------------------------------------------
 *  User response
 *-----------------------------------------------------------------------------*/
int incoming_byte_ = 0;
bool reboot_ = false;

unsigned long currentTime( )
{
    return millis() - trial_start_time_;
}

/*-----------------------------------------------------------------------------
 *  WATCH DOG
 *-----------------------------------------------------------------------------*/
/**
 * @brief Interrupt serviving routine.
 *
 * @param _vect
 */
ISR(WDT_vect)
{
    // Handle interuppts here.
    // Nothing to handle.
}

void reset_watchdog( )
{
    if( not reboot_ )
        wdt_reset( );
}


/**
 * @brief  Read a command from command line. Consume when character is matched.
 *
 * @param command
 *
 * @return False when not mactched. If first character is matched, it is
 * consumed, second character is left in  the buffer.
 */
bool is_command_read( char command, bool consume = true )
{
    if( ! Serial.available() )
        return false;

    // Peek for the first character.
    if( command == Serial.peek( ) )
    {
        if( consume )
            Serial.read( );
        return true;
    }

    return false;
}

/**
 * @brief Write data line to Serial port.
 *   NOTE: Use python dictionary format. It can't be written at baud rate of
 *   38400 at least.
 * @param data
 * @param timestamp
 */
void write_data_line( )
{
    reset_watchdog( );

    int puff = digitalRead( PUFF_PIN );
    int led = digitalRead( LED_PIN );

    int tone = digitalRead( TONE_PIN );

    int microscope = digitalRead( IMAGING_TRIGGER_PIN );
    unsigned camera = digitalRead( CAMERA_TTL_PIN );

    unsigned long timestamp = millis() - trial_start_time_;
    
    sprintf(msg_  
            , "%lu,%d,%d,%d,%d,%d,%d,%d,%d,%s"
            , timestamp, trial_count_, puff, tone, led
            , motion1, motion2, camera, microscope, trial_state_
            );
    Serial.println(msg_);
    Serial.flush( );
}

void check_for_reset( void )
{
    if( is_command_read( 'r', true ) )
    {
        Serial.println( ">>> Reboot in 2 seconds" );
        reboot_ = true;
    }
}

/**
 * @brief Play tone for given period and duty cycle. 
 *
 * NOTE: We can not block the arduino using delay, since we have to write the
 * values onto Serial as well.
 *
 * @param period
 * @param duty_cycle
 */
void play_tone( unsigned long period, double duty_cycle = 0.5 )
{
    reset_watchdog( );
    check_for_reset( );
    unsigned long toneStart = millis();
    while( millis() - toneStart <= period )
    {
        write_data_line();
        if( millis() - toneStart <= (period * duty_cycle) )
            tone( TONE_PIN, TONE_FREQ );
        else
            noTone( TONE_PIN );
    }
}


/**
 * @brief Play puff for given duration.
 *
 * @param duration
 */
void play_puff( unsigned long duration )
{
    check_for_reset( );
    stamp_ = millis();
    while( millis() - stamp_ <= duration )
    {
        digitalWrite( PUFF_PIN, HIGH);
        write_data_line( );
    }
    digitalWrite( PUFF_PIN, LOW );
    write_data_line( );
}


/**
 * @brief Configure the experiment here. All parameters needs to be set must be
 * done here.
 */
void configure_experiment( )
{
    // While this is not answered, keep looping 
    Serial.println( "?? Please configure your experiment" );
    Serial.println( "NOTE: This has been disabled" );
    while( true )
    {
        break;
#if 0
        incoming_byte_ = Serial.read( ) - '0';
        if( incoming_byte_ < 0 )
        {
            Serial.println( ">>> ... Waiting for response" );
            delay( 1000 );
        }
        else if( incoming_byte_ == 0 )
        {
            Serial.print( ">>> Valid response. Got " );
            Serial.println( incoming_byte_  );
            Serial.flush( );
            tsubtype_ = first;
            return;
        }
        else if( incoming_byte_ == 1 )
        {
            Serial.print( ">>> Valid response. Got " );
            Serial.println( incoming_byte_ );
            Serial.flush( );
            tsubtype_ = second;
            return;
        }
        else
        {
            Serial.print( ">>> Unexpected response, recieved : " );
            Serial.println( incoming_byte_ - '0' );
            delay( 100 );
        }
#endif
    }
}


/**
 * @brief Wait for trial to start.
 */
void wait_for_start( )
{
    sprintf( trial_state_, "INVA" );
    while( true )
    {
        write_data_line( );
        if( is_command_read( 's', true ) )
        {
            Serial.println( ">>> Start" );
            break;
        }
        else if( is_command_read( 'p', true ) ) 
        {
            Serial.println( ">>> Playing puff" );
            play_puff( PUFF_DURATION );
        }
        else if( is_command_read( 't', true ) ) 
        {
            Serial.println( ">>> Playing tone" );
            play_tone( TONE_DURATION, 1.0);
        }
        else
        {
            //Serial.print( "Got this character " );
            Serial.print( Serial.read( ) );
        }
    }
}


void setup()
{
    Serial.begin( 38400 );

    // setup watchdog. If not reset in 2 seconds, it reboots the system.
    wdt_enable( WDTO_2S );
    wdt_reset();
    stamp_ = 0;

    pinMode( TONE_PIN, OUTPUT );
    pinMode( PUFF_PIN, OUTPUT );
    pinMode( CAMERA_TTL_PIN, OUTPUT );
    pinMode( IMAGING_TRIGGER_PIN, OUTPUT );

    // When HIGH, imaging starts, when LOW imaging stops.
    pinMode( IMAGING_TRIGGER_PIN, OUTPUT);

    digitalWrite( PUFF_PIN, LOW );
    digitalWrite( IMAGING_TRIGGER_PIN, LOW);

    tone( TONE_PIN, 0 );

    configure_experiment( );
    Serial.println( ">>> Waiting for 's' to be pressed" );
    wait_for_start( );
}

void do_zero_trial( )
{
    return;
}

void do_first_trial( )
{
    return;
}

/**
 * @brief Do a single trial.
 *
 * @param trial_num. Index of the trial.
 * @param ttype. Type of the trial.
 */
void do_trial( unsigned int trial_num, bool play_tone = false )
{
    reset_watchdog( );
    check_for_reset( );

    trial_start_time_ = millis( );

    /*-----------------------------------------------------------------------------
     *  PRE
     *-----------------------------------------------------------------------------*/
    unsigned duration = 500;
    unsigned endBlockTime = 0;

    sprintf( trial_state_, "PRE_" );
    digitalWrite( IMAGING_TRIGGER_PIN, HIGH);

    write_data_line( );
    while( millis( ) - trial_start_time_ < duration ) /* PRE_ time */
        write_data_line( );

    /*-----------------------------------------------------------------------------
     *  CS: 50 ms duration. No tone is played here. Write LED pin to HIGH.
     *-----------------------------------------------------------------------------*/
    endBlockTime = millis( );
    duration = 50;
    while( millis( ) -  endBlockTime <= duration )
    {
        check_for_reset( );
        write_data_line( );
        digitalWrite( LED_PIN, HIGH );
    }
    endBlockTime = millis( );


    /*-----------------------------------------------------------------------------
     *  TRACE. The duration of trace varies from trial to trial.
     *-----------------------------------------------------------------------------*/
    if( 6 <= trial_num <= 7 )
        duration = 0;
    else if( 10 <= trial_num <= 11 )
        duration = 350;
    else if( 12 <= trial_num <= 13 )
        duration = 450;
    else
        duration = 250;

    sprintf( trial_state_, "TRAC" );
    while( millis( ) - endBlockTime <= duration )
    {
        check_for_reset( );
        write_data_line( );
    }
    endBlockTime = millis( );

    /*-----------------------------------------------------------------------------
     *  PUFF for 50 ms.
     *-----------------------------------------------------------------------------*/
    duration = 50;
    sprintf( trial_state_, "PUFF" );
    play_puff( duration );
    endBlockTime = millis( );
    
    /*-----------------------------------------------------------------------------
     *  POST, flexible duration till trial is over.
     *-----------------------------------------------------------------------------*/
    // Last phase is post. If we are here just spend rest of time here.
    sprintf( trial_state_, "POST" );
    while( millis( ) - trial_start_time_ <= TRIAL_DURATION )
    {
        check_for_reset( );
        write_data_line( );
    }

    /*-----------------------------------------------------------------------------
     *  End trial.
     *-----------------------------------------------------------------------------*/
    if( millis() - trial_start_time_ >= TRIAL_DURATION )
    {
        digitalWrite( IMAGING_TRIGGER_PIN, LOW ); /* Shut down the imaging. */
        Serial.print( ">>END Trial " );
        Serial.print( trial_count_ );
        Serial.println( " is over. Starting new");
        trial_count_ += 1;
    }
}

void loop()
{
    reset_watchdog( );

    do_zero_trial( );

    do_first_trial( );

    for (size_t i = 2; i <= 100; i++) 
    {
        reset_watchdog( );
        do_trial( i, false );

        
        /*-----------------------------------------------------------------------------
         *  ITI.
         *-----------------------------------------------------------------------------*/
        unsigned long duration = random( 5000, 10001);
        unsigned long stamp_ = millis( );
        sprintf( trial_state_, "ITI_" );
        while( millis( ) - stamp_ <= duration )
            write_data_line( );
        
    }

    // We are done with all trials. Nothing to do.
    reset_watchdog( );
    Serial.println( "All done. Party!" );
    Serial.flush( );
    delay( 100 );
    exit( 0 );
}
