#include <linux/init.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <config/pg_typedefs.h>
#include <motor/tmi8150b.h>
#include <motor/motor_usr.h>

static int tmi8150_send_data(tmi8150_attri_s *tmi8150, const reg_t *s_reg)
{
	int ret = 0;

	if (!(tmi8150->write_data)) {
		printk_err("write_data function error.\n");
		
		return -1;
	}
	
	ret = tmi8150->write_data(tmi8150->handle, s_reg);
	if (ret < 0)
		printk_err("write data error.\n");
	
	return ret;	
}

static int tmi8150_rcv_data(tmi8150_attri_s *tmi8150, const reg_t *s_reg, const buf_t *r_buf)
{
	int ret = 0;

	if (!(tmi8150->read_data)) {
		printk_err("read_data function error.\n");
		
		return -1;
	}
		
	ret = tmi8150->read_data(tmi8150->handle, s_reg, r_buf);
	if (ret < 0)
		printk_err("read data error.\n");
	
	return ret;	
}

static int tmi8150_write_reg(tmi8150_attri_s *tmi8150, uint8_t reg_addr, uint8_t data)
{
	int ret 			= 0;
	uint8_t msg_data[2] = {0x00};
	reg_t s_reg;
	
	msg_data[0] = reg_addr;
	msg_data[1] = data;
	s_reg.buf = msg_data;
	s_reg.len = 2;
	
	ret = tmi8150_send_data(tmi8150, &s_reg);
	if (ret < 0) {
		printk_err("tmi8150 read data error.\n");
		return -1;
	}

	return ret;
}

static int tmi8150_read_reg(tmi8150_attri_s *tmi8150, uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
	int ret = 0;
	reg_t s_reg;
	buf_t r_buf;
	
	s_reg.buf = &reg_addr;
	s_reg.len = 1;
	r_buf.buf = buf;
	r_buf.len = len;
	
	ret = tmi8150_rcv_data(tmi8150, &s_reg, &r_buf);
	if (ret < 0) {
		printk_err("tmi8150 read data error.\n");
		return -1;
	}
	
	return ret;
}

static int tmi8150_get_id(tmi8150_attri_s *tmi8150)
{
	uint8_t retry 				= 0;
	uint8_t tmi8150_chip_id0 	= 0x00;
	uint8_t tmi8150_chip_id1 	= 0x00;

	while((tmi8150_chip_id0 != 0x81) && (tmi8150_chip_id1 != 0x50) && (retry++ < 5)) {
		tmi8150_read_reg(tmi8150, Tmi8150Register_CHIP_ID0, &tmi8150_chip_id0, 1);
		tmi8150_read_reg(tmi8150, Tmi8150Register_CHIP_ID1, &tmi8150_chip_id1, 1);
		printk_log("error Tmi8150Register_CHIP_ID = 0x%x, 0x%x\n", tmi8150_chip_id0, tmi8150_chip_id1);
	}
	
	if(retry > 5) {
		printk_err("error in reading chip id by spi.\n");
		return -1;
	}

	return 0;
}

static void tmi8150_config(tmi8150_attri_s *tmi8150)
{
	tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_GCTRL, 0x03);
	tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_GCTRL, 0x8B);
	
	if (tmi8150->x_axis_enable) {
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CN, 0x07);
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_MD0, 0x75);
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_MD1, 0x06);
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYCNT0, 0x00);
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYCNT1, 0x00);
	}

	if (tmi8150->y_axis_enable) {
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CN, 0x07);
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_MD0, 0x75);
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_MD1, 0x06);
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYCNT0, 0x00);
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYCNT1, 0x00);
	}

	return;
}

static void tmi8150_run_config(tmi8150_attri_s *tmi8150)
{
	uint8_t r_ch_md0 		= 0;
	uint8_t cur_r_step[2] 	= {0x00};
	uint16_t cur_step 		= 0;
	uint16_t cur_step_tmp 	= 0;

	if (tmi8150->x_axis_enable) {
		tmi8150_read_reg(tmi8150, 0x80 | Tmi8150Register_CH12_MD0, &r_ch_md0, 1);
		r_ch_md0 &= 0xF0;
		if (tmi8150->m_status.xaxis_status) {
			tmi8150_read_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYCNT0, cur_r_step, 2);
			cur_step = (cur_r_step[1] << 8) | cur_r_step[0];
			if (tmi8150->m_status.xaxis_direction) {
				if (tmi8150->m_status.xstep_diff_steps > (MAX_STEP - cur_step)) {
					tmi8150->m_status.xstep_loop_times = (tmi8150->m_status.xstep_diff_steps / MAX_STEP) + 1;
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET0, 0xFF);
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET1, 0x1F);
				} else {
					tmi8150->m_status.xstep_loop_times = 0;
					cur_step_tmp = cur_step + tmi8150->m_status.xstep_diff_steps;
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET0, (uint8_t)(cur_step_tmp));
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET1, (uint8_t)(cur_step_tmp >> 8));			
				}
				r_ch_md0 |= CLK_DIR;	
			} else {
				if (tmi8150->m_status.xstep_diff_steps > cur_step) {
					tmi8150->m_status.xstep_loop_times = (tmi8150->m_status.xstep_diff_steps / MAX_STEP) + 1;
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET0, 0x00);
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET1, 0x00);
				} else {
					tmi8150->m_status.xstep_loop_times = 0;
					cur_step_tmp = cur_step - tmi8150->m_status.xstep_diff_steps;
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET0, (uint8_t)(cur_step_tmp));
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET1, (uint8_t)(cur_step_tmp >> 8));			
				}
				r_ch_md0 |= ANT_CLK_DIR;
			}
		} else {
			r_ch_md0 |= NOW_STOP;
		}
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_MD0, r_ch_md0);
	}

	if (tmi8150->y_axis_enable) {
		tmi8150_read_reg(tmi8150, 0x80 | Tmi8150Register_CH34_MD0, &r_ch_md0, 1);
		r_ch_md0 &= 0xF0;
		if (tmi8150->m_status.yaxis_status) {
			tmi8150_read_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYCNT0, cur_r_step, 2);
			cur_step = (cur_r_step[1] << 8) | cur_r_step[0];
			if (tmi8150->m_status.yaxis_direction) {
				if (tmi8150->m_status.ystep_diff_steps > (MAX_STEP - cur_step)) {
					tmi8150->m_status.ystep_loop_times = (tmi8150->m_status.ystep_diff_steps / MAX_STEP) + 1;
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET0, 0xFF);
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET1, 0x1F);
				} else {
					tmi8150->m_status.ystep_loop_times = 0;
					cur_step_tmp = cur_step + tmi8150->m_status.ystep_diff_steps;
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET0, (uint8_t)(cur_step_tmp));
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET1, (uint8_t)(cur_step_tmp >> 8));			
				}
				r_ch_md0 |= CLK_DIR;	
			} else {
				if (tmi8150->m_status.ystep_diff_steps > cur_step) {
					tmi8150->m_status.ystep_loop_times = (tmi8150->m_status.ystep_diff_steps / MAX_STEP) + 1;
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET0, 0x00);
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET1, 0x00);
				} else {
					tmi8150->m_status.ystep_loop_times = 0;
					cur_step_tmp = cur_step - tmi8150->m_status.ystep_diff_steps;
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET0, (uint8_t)(cur_step_tmp));
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET1, (uint8_t)(cur_step_tmp >> 8));			
				}
				r_ch_md0 |= ANT_CLK_DIR;
			}
		} else {
			r_ch_md0 |= NOW_STOP;
		}
		tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_MD0, r_ch_md0);
	}

	return;
}

void get_ptz_state(tmi8150_attri_s *tmi8150, unsigned long arg)
{
    struct motor_param_get mparam;
    
    if(copy_from_user(&mparam, (void *)arg, sizeof(struct motor_param_get))) {
		printk_err("copy mparam data error\n");
		return;
    }
	
    switch(mparam.state_cmd)
    {
        case GET_XAXIS_POS:
            mparam.state_value	= tmi8150->m_status.xaxis_cur_pos;
            break;
        case GET_YAXIS_POS:
            mparam.state_value	= tmi8150->m_status.yaxis_cur_pos;
            break;
        case GET_XAXIS_CHECK_STATE:
			mparam.state_value	= !(tmi8150->m_status.xaxis_cur_pos);
			break;		
        default:
            break;

    }

    if(copy_to_user((void *)arg, &mparam, sizeof(struct motor_param_get))) {
		printk_err("copy to mparam data error\n");
        return;
    }

    return;
}

void set_ptz_cmd(tmi8150_attri_s *tmi8150, unsigned long arg)
{
    struct motor_param_set mparam;

	memset(&mparam, 0x00, sizeof(struct motor_param_set));
    if(copy_from_user(&mparam, (void *)arg, sizeof(struct motor_param_set))) {
        printk_err("copy data error\n");
		
        return;
    }

    switch(mparam.motor_func) {
        case GO_UP:
            tmi8150->m_status.yaxis_status 		= 1;
            tmi8150->m_status.yaxis_goal_pos 	= tmi8150->yaxis_step_max;
			tmi8150->m_status.yaxis_direction 	= 1;
            break;
        case GO_DOWN:
            tmi8150->m_status.yaxis_status 		= 1;
            tmi8150->m_status.yaxis_goal_pos 	= 0;
            tmi8150->m_status.yaxis_direction 	= 0;
            break;
        case GO_LEFT:
            tmi8150->m_status.xaxis_status 		= 1;
            tmi8150->m_status.xaxis_goal_pos	= 0;
            tmi8150->m_status.xaxis_direction 	= 0;
            break;
        case GO_RIGHT:			
            tmi8150->m_status.xaxis_status 		= 1;
            tmi8150->m_status.xaxis_goal_pos 	= tmi8150->xaxis_step_max;
            tmi8150->m_status.xaxis_direction 	= 1;
            break;
        case RIGHT_UP:
            tmi8150->m_status.xaxis_status 		= 1;
            tmi8150->m_status.yaxis_status 		= 1;
			tmi8150->m_status.xaxis_goal_pos 	= tmi8150->xaxis_step_max;
			tmi8150->m_status.yaxis_goal_pos 	= tmi8150->yaxis_step_max;
            tmi8150->m_status.xaxis_direction 	= 1;
            tmi8150->m_status.yaxis_direction 	= 1;
            break;
        case RIGHT_DOWN:
            tmi8150->m_status.xaxis_status 		= 1;
            tmi8150->m_status.yaxis_status 		= 1;
	   		tmi8150->m_status.xaxis_goal_pos 	= tmi8150->xaxis_step_max;
	    	tmi8150->m_status.yaxis_goal_pos 	= 0;
            tmi8150->m_status.xaxis_direction 	= 1;
            tmi8150->m_status.yaxis_direction 	= 0;
            break;
        case LEFT_UP:
            tmi8150->m_status.xaxis_status 		= 1;
            tmi8150->m_status.yaxis_status 		= 1;
			tmi8150->m_status.xaxis_goal_pos 	= 0;
			tmi8150->m_status.yaxis_goal_pos 	= tmi8150->yaxis_step_max;
            tmi8150->m_status.xaxis_direction 	= 0;
            tmi8150->m_status.yaxis_direction 	= 1;
            break;
        case LEFT_DOWN:
            tmi8150->m_status.xaxis_status 		= 1;
            tmi8150->m_status.yaxis_status 		= 1;
			tmi8150->m_status.xaxis_goal_pos 	= 0;
			tmi8150->m_status.yaxis_goal_pos 	= 0;
            tmi8150->m_status.xaxis_direction 	= 0;
            tmi8150->m_status.yaxis_direction 	= 0;
            break;
        case GO_POS:
            tmi8150->m_status.xaxis_goal_pos 	= mparam.goal_xaxis_pos;
            tmi8150->m_status.yaxis_goal_pos 	= mparam.goal_yaxis_pos;
			
            printk_dbg("cur, x_step=%d, goal, x_step=%d\n", tmi8150->m_status.xaxis_cur_pos, tmi8150->m_status.xaxis_goal_pos);
			printk_dbg("cur, y_step=%d; goal, y_step=%d\n", tmi8150->m_status.yaxis_cur_pos, tmi8150->m_status.yaxis_goal_pos);

			if (tmi8150->m_status.xaxis_goal_pos > tmi8150->xaxis_step_max)
				tmi8150->m_status.xaxis_goal_pos = tmi8150->xaxis_step_max;
			if (tmi8150->m_status.xaxis_goal_pos < 0)
				tmi8150->m_status.xaxis_goal_pos = 0;
			
			if (tmi8150->m_status.yaxis_goal_pos > tmi8150->yaxis_step_max)
				tmi8150->m_status.yaxis_goal_pos = tmi8150->yaxis_step_max;
			if (tmi8150->m_status.yaxis_goal_pos < 0)
				tmi8150->m_status.yaxis_goal_pos = 0;

			if (tmi8150->m_status.xaxis_goal_pos == tmi8150->m_status.xaxis_cur_pos) {
				tmi8150->m_status.xaxis_status 		= 0;
			}else {
				tmi8150->m_status.xaxis_status 		= 1;
				if (tmi8150->m_status.xaxis_goal_pos < tmi8150->m_status.xaxis_cur_pos) {	/*left*/
					tmi8150->m_status.xstep_diff_steps	= tmi8150->m_status.xaxis_cur_pos - tmi8150->m_status.xaxis_goal_pos;
					tmi8150->m_status.xaxis_direction 	= 0;
				}else {															/*right*/
					tmi8150->m_status.xstep_diff_steps	= tmi8150->m_status.xaxis_goal_pos - tmi8150->m_status.xaxis_cur_pos;
					tmi8150->m_status.xaxis_direction 	= 1;
				}
			}
			
			if (tmi8150->m_status.yaxis_goal_pos == tmi8150->m_status.yaxis_cur_pos) {
				tmi8150->m_status.yaxis_status 		= 0;
			}else {
				tmi8150->m_status.yaxis_status 		= 1;
				if (tmi8150->m_status.yaxis_goal_pos < tmi8150->m_status.yaxis_cur_pos) {	/*up*/
					tmi8150->m_status.ystep_diff_steps	= tmi8150->m_status.yaxis_cur_pos - tmi8150->m_status.yaxis_goal_pos;
					tmi8150->m_status.yaxis_direction 	= 0;
				}else {
					tmi8150->m_status.ystep_diff_steps	= tmi8150->m_status.yaxis_cur_pos - tmi8150->m_status.yaxis_goal_pos;																/*down*/
					tmi8150->m_status.yaxis_direction 	= 1;
				}
			}
            break;
        case XAXIS_SELF_CHECK:
            tmi8150->m_status.xaxis_status 			= 1;
			tmi8150->m_status.xaxis_check_status	= 1;
            tmi8150->m_status.xaxis_goal_pos 		= 0;
			tmi8150->m_status.xaxis_direction 		= 0;
			tmi8150->m_status.xaxis_cur_pos 		= tmi8150->xaxis_step_max;
            break;
        case YAXIS_SELF_CHECK:		
            tmi8150->m_status.yaxis_status 			= 1;
            tmi8150->m_status.yaxis_goal_pos 		= 0;
			tmi8150->m_status.yaxis_direction 		= 0;
            tmi8150->m_status.yaxis_cur_pos 		= tmi8150->yaxis_step_max;
            break;
        case XAXIS_STOP:
            tmi8150->m_status.xaxis_status 		= 0;
            break;
        case YAXIS_STOP:
            tmi8150->m_status.yaxis_status 		= 0;
            break;
		case BOTH_STOP:
			tmi8150->m_status.xaxis_status 		= 0;
			tmi8150->m_status.yaxis_status 		= 0;
			break;
        default:
            printk_err("invalid argument\n");
            return;
    }

	tmi8150_run_config(tmi8150);

	if (tmi8150->m_status.xaxis_status | tmi8150->m_status.yaxis_status) {
		tmi8150->kt = ktime_set(0, (tmi8150->base_period) * 1000);
	    hrtimer_start(&tmi8150->ms_timer, tmi8150->kt, HRTIMER_MODE_REL);
	}

	return;
}

static void set_init_ptz_cmd(tmi8150_attri_s *tmi8150, unsigned long arg)
{
    struct motor_param_init iparam;

	memset(&iparam, 0x00, sizeof(struct motor_param_init));
    if(copy_from_user(&iparam, (void *)arg, sizeof(struct motor_param_init))) {
        printk_err("copy iparam data error\n");
        return;
    }

	tmi8150->xaxis_step_max	= iparam._xaxis_step_max;
	tmi8150->yaxis_step_max	= iparam._yaxis_step_max;

	printk_log("now, xaxis_max_step=%d, yaxis_max_step=%d\n", tmi8150->xaxis_step_max, tmi8150->yaxis_step_max);	
       
	return;
}

enum hrtimer_restart tmi8150_ms_timer_func(struct hrtimer *timer)
{
	uint8_t cur_r_step[2] = {0x00};
	uint8_t cur_g_step[2] = {0x00};
	uint16_t cur_step_tmp = 0;
	struct tmi8150_attri *tmi8150 = container_of(timer, struct tmi8150_attri, ms_timer);

	if (tmi8150->x_axis_enable) {
		tmi8150_read_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYCNT0, cur_r_step, 2);
		tmi8150_read_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET0, cur_g_step, 2);
		if ((tmi8150->m_status.xaxis_status) && (cur_r_step == cur_g_step)) {
			if (tmi8150->m_status.xstep_loop_times == 0) {
				tmi8150->m_status.xaxis_status 	= 0;
			} else {
				tmi8150->m_status.xstep_loop_times--;
				tmi8150->m_status.xstep_diff_steps -= MAX_STEP;
				tmi8150->m_status.xstep_diff_steps++;				//寄存器从0xFF到0x00，少一步，需要补充	
				if (tmi8150->m_status.xaxis_direction) {		
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYCNT0, 0x00);
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYCNT1, 0x00);
					if (tmi8150->m_status.xstep_diff_steps > MAX_STEP) {
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET0, 0xFF);
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET1, 0x1F);
					} else {
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET0, (uint8_t)(tmi8150->m_status.xstep_diff_steps));
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET1, (uint8_t)(tmi8150->m_status.xstep_diff_steps >> 8));
					}
				} else {
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYCNT0, 0xFF);
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYCNT1, 0x1F);
					if (tmi8150->m_status.xstep_diff_steps > MAX_STEP) {
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET0, 0x00);
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET1, 0x00);
					} else {
						cur_step_tmp = MAX_STEP - tmi8150->m_status.ystep_diff_steps;
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET0, (uint8_t)(cur_step_tmp));
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH12_CYSET1, (uint8_t)(cur_step_tmp >> 8));
					}
				}
			}
		}
	}

	if (tmi8150->y_axis_enable) {
		tmi8150_read_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYCNT0, cur_r_step, 2);
		tmi8150_read_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET0, cur_g_step, 2);
		if ((tmi8150->m_status.yaxis_status) && (cur_r_step == cur_g_step)) {
			if (tmi8150->m_status.ystep_loop_times == 0) {
				tmi8150->m_status.yaxis_status 	= 0;
			} else {
				tmi8150->m_status.ystep_loop_times--;
				tmi8150->m_status.ystep_diff_steps -= MAX_STEP;
				tmi8150->m_status.ystep_diff_steps++;				//寄存器从0xFF到0x00，少一步，需要补充	
				if (tmi8150->m_status.yaxis_direction) {		
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYCNT0, 0x00);
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYCNT1, 0x00);
					if (tmi8150->m_status.ystep_diff_steps > MAX_STEP) {
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET0, 0xFF);
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET1, 0x1F);
					} else {
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET0, (uint8_t)(tmi8150->m_status.ystep_diff_steps));
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET1, (uint8_t)(tmi8150->m_status.ystep_diff_steps >> 8));
					}
				} else {
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYCNT0, 0xFF);
					tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYCNT1, 0x1F);
					if (tmi8150->m_status.ystep_diff_steps > MAX_STEP) {
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET0, 0x00);
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET1, 0x00);
					} else {
						cur_step_tmp = MAX_STEP - tmi8150->m_status.ystep_diff_steps;
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET0, (uint8_t)(cur_step_tmp));
						tmi8150_write_reg(tmi8150, 0x80 | Tmi8150Register_CH34_CYSET1, (uint8_t)(cur_step_tmp >> 8));
					}
				}
			}
		}
	}

    if (tmi8150->m_status.yaxis_status == 0 && tmi8150->m_status.xaxis_status == 0) {
		return HRTIMER_NORESTART;
    }
   
    tmi8150->kt = ktime_set(0, (tmi8150->base_period) * 1000);
    hrtimer_forward_now(timer, tmi8150->kt);

    return HRTIMER_RESTART;
}

static void tmi8150_init_handle(tmi8150_attri_s *tmi8150)
{	
	if (SPI_EMULATE == tmi8150->oper_mode) {
		tmi8150->write_data = spi_emulate_write_data;
		tmi8150->read_data 	= spi_emulate_read_data;
	} else {
		tmi8150->write_data = spi_module_write_data;
	}   tmi8150->read_data 	= spi_module_read_data;
	
	if (tmi8150->spi) {
		tmi8150->handle = (void *)tmi8150->spi;
	} else {
		tmi8150->handle = (void *)tmi8150->emu_spi;
	}

	return;
}

int tmi8150_open(void *chip)
{
	int ret = 0;
	
	tmi8150_attri_s *tmi8150_one = (tmi8150_attri_s *)chip;
	
	tmi8150_init_handle(tmi8150_one);
	ret = tmi8150_get_id(tmi8150_one);
	if (ret < 0) {
		printk("tmi8150_init error.\n");
		
		return -1;
	}
	
	tmi8150_config(tmi8150_one);

	return 0;
}

int tmi8150_close(void *chip)
{
	int ret = 0;
//	tmi8150_attri_s *tmi8150_one = (tmi8150_attri_s *)chip;
	
	return ret;
}

int tmi8150_read(void *chip, char *buf, size_t count)
{
	int ret = 0;
//	tmi8150_attri_s *tmi8150_one = (tmi8150_attri_s *)chip;
	
	return ret;
}

long tmi8150_ioctl(void *chip, uint32_t cmd, unsigned long arg)
{
	tmi8150_attri_s *tmi8150_one = (tmi8150_attri_s *)chip;

    switch(cmd) {
        case MOTOR_IOCTL_GET:
            get_ptz_state(tmi8150_one, arg);
            break;
        case MOTOR_IOCTL_SET:
            set_ptz_cmd(tmi8150_one, arg);
            break;
		case MOTOR_IOCTL_PARA_INIT_SET:
			set_init_ptz_cmd(tmi8150_one, arg);
			break;
        default:
			printk_err("invalid cmd.\n");
            break;
    }

    return 0;
}
