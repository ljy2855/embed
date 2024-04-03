
enum INPUT_TYPE
{
    READ_KEY,
    SWITCH,
    RESET,
};

enum KEY_TYPE
{
    BACK,
    PROG,
    VOL_UP,
    VOL_DOWN,
};

enum SWITCH_TYPE
{
    ONE = 1,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    ONE_LONG,
    RESET_SWITCH,
};

typedef struct io_protocol
{
    enum INPUT_TYPE input_type;
    int value;
} io_protocol;