#include <stdio.h>

#include <sapi/sys.hpp>
#include <sapi/hal.hpp>
#include <sapi/var.hpp>

#include "sl_config.h"

static void show_usage(const Cli & cli);
static void execute_read_all(const Cli & cli);
static void execute_read(const Cli & cli, mcu_pin_t pin);
static void execute_write(const Cli & cli, mcu_pin_t pin, int value);
static void execute_mode(const Cli & cli, mcu_pin_t pin, const var::ConstString & mode);
static void execute_pulse(const Cli & cli, mcu_pin_t pin, int value, int duration_us);

int main(int argc, char * argv[]){
	String action;
	String port_pin;
	String is_help;
	String mode;
	String value;
	String duration;
	mcu_pin_t pin;

	Cli cli(argc, argv);
	cli.set_publisher(SL_CONFIG_PUBLISHER);
	cli.handle_version();


	action = cli.get_option("action", "specify the operation read|write|readall|mode|pulse");
	port_pin = cli.get_option("pin", "specify the port/pin combination as X.Y, e.g. --pin=2.0");
	is_help = cli.get_option("help", "show help options");
	mode = cli.get_option("mode", "specify mode as float|pullup|pulldown|out|opendrain");
	value = cli.get_option("value", "specify output value as 0|1 or operations write|pulse");
	duration = cli.get_option("duration", "specify pulse duration in microseconds, e.g. --duration=100");

	pin = Pin::from_string(port_pin);

	if( is_help.is_empty() == false ){
		cli.show_options();
		exit(0);
	}

	if( action == "readall" ){
		execute_read_all(cli);
	} else if ( action == "read" ){
		execute_read(cli, pin);
	} else if ( action == "write" ){
		execute_write(cli, pin, value.to_integer());
	} else if ( action == "mode" ){
		execute_mode(cli, pin, mode);
	} else if ( action == "pulse" ){
		execute_pulse(cli, pin, value.to_integer(), duration.to_integer());
	} else {
		show_usage(cli);
	}


	return 0;
}


void execute_read_all(const Cli & cli){
	int port_num = 0;
	int pin;
	u32 value;
	//show the status of all the pins
	printf("%s:\n", cli.name().cstring());
	printf("       28   24   20   16   12    8    4    0\n");
	printf("     ---- ---- ---- ---- ---- ---- ---- ----\n");
	do {
		Pio p(port_num);

		if( (p.open() < 0) || (port_num > 10) ){
			break;
		}

		printf("P%d | ", port_num);
		for(pin = 0; pin < 32; pin++){
			value = p.get_value();
			if( value & (1<<(31-pin)) ){
				printf("1");
			} else {
				printf("0");
			}

			if( (pin+1) % 4 == 0 ){
				printf(" ");
			}
		}
		printf("\n");

		port_num++;

	} while(1);

	printf("\n");

}

void show_usage(const Cli & cli){
	printf("%s usage:\n", cli.name().cstring());
	cli.show_options();
	exit(1);
}

void execute_read(const Cli & cli, mcu_pin_t pin){

	if( pin.port != 255 ){
		Pin p(pin);
		if( p.open(Pin::RDWR) < 0 ){
			printf("Failed to open /dev/pio%d\n", pin.port);
		} else {
			printf("%s:%d.%d == %d\n", cli.name().cstring(), pin.port, pin.pin, p.get_value());
			p.close();
		}
	} else {
		show_usage(cli);
	}
}

void execute_write(const Cli & cli, mcu_pin_t pin, int value){

	if( pin.port != 255 ){
		Pin p(pin.port, pin.pin);
		if( p.open(Pin::RDWR) < 0 ){
			printf("Failed to open /dev/pio%d\n", pin.port);
		} else {
			if( value == 0 ){
				p.clear();
			} else {
				value = 1;
				p.set();
			}
			printf("%s:%d.%d -> %d\n", cli.name().cstring(), pin.port, pin.pin, value);
			p.close();
		}
	} else {
		show_usage(cli);
	}

}

void execute_mode(const Cli & cli, mcu_pin_t pin, const var::ConstString & mode){

	if( mode.is_empty() ){
		show_usage(cli);
		exit(1);
	}

	if( pin.port == 255 ){
		show_usage(cli);
		exit(1);
	} else {
		Pin p(pin.port, pin.pin);
		if( p.open(Pin::READWRITE) < 0 ){
			printf("Failed to open /dev/pio%d", pin.port);
		} else {
			if( (mode == "in") || (mode == "float") || (mode == "tri") ){
				p.set_attributes(Pin::FLAG_SET_INPUT | Pin::FLAG_IS_FLOAT);
				printf("%s:%d.%d -> in\n", cli.name().cstring(), pin.port, pin.pin);
			} else if ( mode == "out" ){
				p.set_attributes(Pin::FLAG_SET_OUTPUT);
				printf("%s:%d.%d -> out\n", cli.name().cstring(), pin.port, pin.pin);
			} else if ( (mode == "up") || (mode == "pullup") ){
				p.set_attributes(Pin::FLAG_SET_INPUT | Pin::FLAG_IS_PULLUP);
				printf("%s:%d.%d -> pullup\n", cli.name().cstring(), pin.port, pin.pin);
			} else if ( (mode == "down") || (mode == "pulldown") ){
				p.set_attributes(Pin::FLAG_SET_INPUT | Pin::FLAG_IS_PULLDOWN);
				printf("%s:%d.%d -> pulldown\n", cli.name().cstring(), pin.port, pin.pin);
			}
		}
	}
}

void execute_pulse(const Cli & cli, mcu_pin_t pin, int value, int duration_us){
	if( pin.port == 0xff ){
		show_usage(cli);
	} else {
		Pin p(pin.port, pin.pin);
		if( p.open(Pin::READWRITE) < 0 ){
			printf("Failed to open /dev/pio%d", pin.port);
		} else {
			//go high then low
			printf("%s:%d.%d -> %d (%dusec) -> %d\n", cli.name().cstring(), pin.port, pin.pin, value, duration_us, !value);
			p = (value != 0);
			Timer::wait_microseconds(duration_us);
			p = (value == 0);
		}
	}
}


