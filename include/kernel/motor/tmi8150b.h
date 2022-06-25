#ifndef __TMI8150B_H__
#define __TMI8150B_H__

#include <linux/hrtimer.h>
#include <config/pg_spi.h>
#include <linux/delay.h>

#define MAX_STEP	0x1FFF
#define CLK_DIR		0x05
#define ANT_CLK_DIR 0x0A
#define NOW_STOP	0x0F

enum Tmi8150Register
{
	Tmi8150Register_CHIP_ID0 = 0,
	Tmi8150Register_CHIP_ID1,
	Tmi8150Register_GCTRL,
	Tmi8150Register_GCCON,
	Tmi8150Register_CH12_CN,
	Tmi8150Register_CH12_MD0,
	Tmi8150Register_CH12_MD1,
	Tmi8150Register_CH12_PHASE,
	Tmi8150Register_CH12_CYCNT0 = 0x08,
	Tmi8150Register_CH12_CYCNT1,
	Tmi8150Register_CH12_PHSET,
	Tmi8150Register_CH12_CYSET0,
	Tmi8150Register_CH12_CYSET1,
	Tmi8150Register_CH1_PULSE,
	Tmi8150Register_CH2_PULSE,
	Tmi8150Register_CH12_PHASEH,
	Tmi8150Register_CH12_PULSEH = 0x10,
	Tmi8150Register_CHIR_CN,
	Tmi8150Register_CH34_PHASEH,
	Tmi8150Register_CH34_PULSEH, 
	Tmi8150Register_CH34_CN,
	Tmi8150Register_CH34_MD0,
	Tmi8150Register_CH34_MD1,
	Tmi8150Register_CH34_PHASE,
	Tmi8150Register_CH34_CYCNT0 = 0x18,
	Tmi8150Register_CH34_CYCNT1,
	Tmi8150Register_CH34_PHSET,
	Tmi8150Register_CH34_CYSET0,
	Tmi8150Register_CH34_CYSET1,
	Tmi8150Register_CH3_PULSE,
	Tmi8150Register_CH4_PULSE,
	Tmi8150Register_CH_PWMSET
};

typedef struct motor_status {
    uint8_t xaxis_status;			//axis status. running or not?
    uint8_t yaxis_status;		
    uint8_t xaxis_direction;		//axis direction. left or right. up or down.
    uint8_t yaxis_direction;
	uint8_t	xaxis_check_status;		//self check status when starts. finish or not.
    int 	xaxis_cur_pos;			//current position
    int 	yaxis_cur_pos;
	int		xaxis_goal_pos;			//goal position
	int		yaxis_goal_pos;

	int 	xstep_diff_steps;
	int		xstep_loop_times;
	int 	ystep_diff_steps;
	int		ystep_loop_times;	
}motor_status_t;

typedef struct tmi8150_attri {
	gen_spi_oper_mode_e oper_mode;
	uint8_t x_axis_enable;
	uint8_t y_axis_enable;

	ktime_t 			kt;
	struct hrtimer 		ms_timer;
	uint32_t			base_period;
	
	motor_status_t m_status;

	int	xaxis_step_max;
	int	yaxis_step_max;

	void *handle;
	struct spi_device *spi;
	struct spi_emulate *emu_spi;
	
	int (*write_data)(void *handle, const reg_t *s_reg);
	int (*read_data)(void *handle, const reg_t *s_reg, const buf_t *r_buf);
}tmi8150_attri_s;

enum hrtimer_restart tmi8150_ms_timer_func(struct hrtimer *timer);

int tmi8150_open(void *chip);
int tmi8150_close(void *chip);
int tmi8150_read(void *chip, char *buf, size_t count);
long tmi8150_ioctl(void *chip, uint32_t cmd, unsigned long arg);

#endif
