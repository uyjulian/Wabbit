#include "stdafx.h"

#include "device.h"

#ifdef DEBUG
void console_output(CPU_t *cpu, device_t *dev) {
	if (cpu->output) {
		printf("output byte: %d\n",cpu->bus);
		cpu->output = FALSE;
	}
}
#endif

void ClearDevices(CPU_t* cpu) {
	int i;
	for (i = 0; i < ARRAYSIZE(cpu->pio.interrupt); i++) {
		cpu->pio.devices[i].active = FALSE;
		cpu->pio.interrupt[i] = -1;
		cpu->pio.skip_factor[i] = 1;
		cpu->pio.skip_count[i] = 0;
	}
	cpu->pio.num_interrupt = 0;
}

int device_output(CPU_t *cpu, unsigned char dev) {
	if (cpu->pio.devices[dev].active) {
		cpu->output = TRUE;
		if (!cpu->pio.devices[dev].protected_port || !cpu->mem_c->flash_locked)
			cpu->pio.devices[dev].code(cpu, &(cpu->pio.devices[dev]));
		if (cpu->pio.devices[dev].breakpoint)
			cpu->pio.breakpoint_callback(cpu, &(cpu->pio.devices[dev]));
		if (cpu->output) {
			/* Device is not responding */
			cpu->output = FALSE;
			return 1;
		}
	}
	return 0;
}

int device_input(CPU_t *cpu, unsigned char dev) {
	if (cpu->pio.devices[dev].active) {
		cpu->input = TRUE;
		if (cpu->pio.devices[dev].breakpoint)
			cpu->pio.breakpoint_callback(cpu, &(cpu->pio.devices[dev]));
		cpu->pio.devices[dev].code(cpu, &(cpu->pio.devices[dev]));
		if (cpu->input) {
			/* Device is not responding */
			cpu->input = FALSE;
			cpu->bus = 0xFF;
			return 1;
		}
	} else  {
		cpu->bus = 0xFF;
		return 1;
	}
	return 0;
}

void Append_interrupt_device(CPU_t *cpu, int port, int skip) {
	cpu->pio.interrupt[cpu->pio.num_interrupt] = port;
	cpu->pio.skip_factor[cpu->pio.num_interrupt] = skip;
	cpu->pio.num_interrupt++;
}

void Modify_interrupt_device(CPU_t *cpu, int port, int skip) {
	int i;
	for(i = 0; i < cpu->pio.num_interrupt; i++) {
		if (cpu->pio.interrupt[i] == port) {
			cpu->pio.skip_factor[i] = skip;
			break;
		}
	}
}

int device_control(CPU_t *cpu, unsigned char dev) {
	device_t *device = &cpu->pio.devices[dev];
	if (device->active) {
		device->code(cpu, device);
	}
	return 0;
}

