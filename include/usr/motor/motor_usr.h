#ifndef __MOTOR_USR_H__
#define __MOTOR_USR_H__

#define MOTOR_MAGIC     'M'

typedef enum
{
    GO_UP,
    RIGHT_UP,
    GO_RIGHT,
    RIGHT_DOWN,
    GO_DOWN,
    LEFT_DOWN,
    GO_LEFT = 6,
    LEFT_UP,

    XAXIS_STOP,
    XAXIS_SELF_CHECK,
    GO_POS,
    YAXIS_STOP,
    YAXIS_SELF_CHECK,
    BOTH_STOP,
    NONE
}PTZ_FUNC;

struct motor_param_set {
    PTZ_FUNC 	motor_func;
	int		 	goal_xaxis_speed;
	int			goal_yaxis_speed;
    int      	goal_xaxis_pos;
    int      	goal_yaxis_pos;
    int         res[5];
};

struct motor_param_init {
	int	_xaxis_step_max;
	int	_yaxis_step_max;
    int res[8];
};

/*current value cmd*/
typedef enum 
{
	GET_XAXIS_POS,
	GET_YAXIS_POS,
	GET_XAXIS_CHECK_STATE,
}PTZ_STATE_CMD;

struct motor_param_get {
	PTZ_STATE_CMD   state_cmd;
	int             state_value;
    int             res[8];
};

#define MOTOR_IOCTL_STOP     		_IO(MOTOR_MAGIC, 0)
#define MOTOR_IOCTL_GET      		_IOR(MOTOR_MAGIC, 1, struct motor_param_get)
#define MOTOR_IOCTL_SET      		_IOW(MOTOR_MAGIC, 2, struct motor_param_set)
#define MOTOR_IOCTL_PARA_INIT_SET   _IOW(MOTOR_MAGIC, 3, struct motor_param_init)

#endif

