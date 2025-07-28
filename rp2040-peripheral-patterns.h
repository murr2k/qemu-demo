/*
 * RP2040 Peripheral Implementation Patterns
 * 
 * This header demonstrates the patterns for implementing RP2040 peripherals
 * following QEMU conventions.
 */

#ifndef HW_ARM_RP2040_PERIPHERALS_H
#define HW_ARM_RP2040_PERIPHERALS_H

#include "hw/sysbus.h"
#include "hw/registerfields.h"
#include "qom/object.h"

/* Example: GPIO Peripheral Pattern */
#define TYPE_RP2040_GPIO "rp2040-gpio"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040GPIOState, RP2040_GPIO)

/* Register definitions using QEMU's FIELD macro */
REG32(GPIO_IN, 0x004)          /* Input value */
REG32(GPIO_OUT, 0x010)         /* Output value */
REG32(GPIO_OUT_SET, 0x014)     /* Output set */
REG32(GPIO_OUT_CLR, 0x018)     /* Output clear */
REG32(GPIO_OUT_XOR, 0x01c)     /* Output XOR */
REG32(GPIO_OE, 0x020)          /* Output enable */
REG32(GPIO_OE_SET, 0x024)      /* Output enable set */
REG32(GPIO_OE_CLR, 0x028)      /* Output enable clear */
REG32(GPIO_OE_XOR, 0x02c)      /* Output enable XOR */

/* Per-pin control registers */
REG32(GPIO_CTRL, 0x00)
    FIELD(GPIO_CTRL, FUNCSEL, 0, 5)    /* Function select */
    FIELD(GPIO_CTRL, OUTOVER, 8, 2)    /* Output override */
    FIELD(GPIO_CTRL, OEOVER, 12, 2)    /* Output enable override */
    FIELD(GPIO_CTRL, INOVER, 16, 2)    /* Input override */
    FIELD(GPIO_CTRL, IRQOVER, 28, 2)   /* IRQ override */

typedef struct RP2040GPIOState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    
    MemoryRegion iomem;
    qemu_irq irq[8];  /* 8 GPIO IRQs */
    qemu_irq output[30]; /* 30 GPIO pins */
    
    /* Registers */
    uint32_t in_value;
    uint32_t out_value;
    uint32_t oe_value;
    
    /* Per-pin control */
    struct {
        uint32_t ctrl;
        uint32_t status;
    } pins[30];
    
    /* Interrupt control */
    uint32_t inte[4];  /* Interrupt enable */
    uint32_t intf[4];  /* Interrupt force */
    uint32_t ints[4];  /* Interrupt status */
} RP2040GPIOState;

/* Example: SIO (Single-cycle I/O) Pattern - Inter-core communication */
#define TYPE_RP2040_SIO "rp2040-sio"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040SIOState, RP2040_SIO)

/* SIO Registers */
REG32(SIO_CPUID, 0x000)              /* Core ID (0 or 1) */
REG32(SIO_GPIO_IN, 0x004)            /* GPIO input */
REG32(SIO_GPIO_HI_IN, 0x008)         /* GPIO input for pins 30+ */
REG32(SIO_GPIO_OUT, 0x010)           /* GPIO output */
REG32(SIO_GPIO_OUT_SET, 0x014)       /* GPIO output set */
REG32(SIO_GPIO_OUT_CLR, 0x018)       /* GPIO output clear */
REG32(SIO_GPIO_OUT_XOR, 0x01c)       /* GPIO output XOR */
REG32(SIO_GPIO_OE, 0x020)            /* GPIO output enable */
REG32(SIO_GPIO_OE_SET, 0x024)        /* GPIO OE set */
REG32(SIO_GPIO_OE_CLR, 0x028)        /* GPIO OE clear */
REG32(SIO_GPIO_OE_XOR, 0x02c)        /* GPIO OE XOR */

/* Inter-processor FIFO */
REG32(SIO_FIFO_ST, 0x050)
    FIELD(SIO_FIFO_ST, VLD, 0, 1)    /* Valid data */
    FIELD(SIO_FIFO_ST, RDY, 1, 1)    /* Ready for data */
    FIELD(SIO_FIFO_ST, WOF, 2, 1)    /* Write overflow */
    FIELD(SIO_FIFO_ST, ROE, 3, 1)    /* Read overflow */

REG32(SIO_FIFO_WR, 0x054)            /* Write to FIFO */
REG32(SIO_FIFO_RD, 0x058)            /* Read from FIFO */

/* Spinlocks */
REG32(SIO_SPINLOCK, 0x100)           /* 32 spinlocks starting here */

typedef struct RP2040SIOState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    
    MemoryRegion iomem[2];  /* Separate view for each core */
    
    /* Inter-processor FIFOs */
    struct {
        uint32_t data[8];   /* 8-deep FIFO */
        int rptr, wptr;
        bool full, empty;
    } fifo[2];  /* One for each direction */
    
    /* Spinlocks */
    uint32_t spinlock[32];
    
    /* GPIO mirror registers */
    uint32_t gpio_out;
    uint32_t gpio_oe;
    
    /* Current core accessing (for CPUID register) */
    int current_cpu;
} RP2040SIOState;

/* Example: PIO (Programmable I/O) Pattern - Unique to RP2040 */
#define TYPE_RP2040_PIO "rp2040-pio"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040PIOState, RP2040_PIO)

/* PIO Registers */
REG32(PIO_CTRL, 0x000)
    FIELD(PIO_CTRL, SM_ENABLE, 0, 4)      /* State machine enable */
    FIELD(PIO_CTRL, SM_RESTART, 4, 4)     /* State machine restart */
    FIELD(PIO_CTRL, CLKDIV_RESTART, 8, 4) /* Clock divider restart */

REG32(PIO_FSTAT, 0x004)
    FIELD(PIO_FSTAT, RXFULL, 0, 4)        /* RX FIFO full */
    FIELD(PIO_FSTAT, RXEMPTY, 8, 4)       /* RX FIFO empty */
    FIELD(PIO_FSTAT, TXFULL, 16, 4)       /* TX FIFO full */
    FIELD(PIO_FSTAT, TXEMPTY, 24, 4)      /* TX FIFO empty */

/* State machine registers */
REG32(PIO_SM_CLKDIV, 0x00)
    FIELD(PIO_SM_CLKDIV, FRAC, 8, 8)      /* Fractional divider */
    FIELD(PIO_SM_CLKDIV, INT, 16, 16)     /* Integer divider */

REG32(PIO_SM_EXECCTRL, 0x04)
    FIELD(PIO_SM_EXECCTRL, STATUS_N, 0, 4)     /* Status select */
    FIELD(PIO_SM_EXECCTRL, STATUS_SEL, 4, 1)   /* Status source */
    FIELD(PIO_SM_EXECCTRL, WRAP_BOTTOM, 7, 5)  /* Wrap bottom */
    FIELD(PIO_SM_EXECCTRL, WRAP_TOP, 12, 5)    /* Wrap top */
    FIELD(PIO_SM_EXECCTRL, OUT_STICKY, 17, 1)  /* Sticky output */
    FIELD(PIO_SM_EXECCTRL, INLINE_OUT_EN, 18, 1)
    FIELD(PIO_SM_EXECCTRL, OUT_EN_SEL, 19, 5)
    FIELD(PIO_SM_EXECCTRL, JMP_PIN, 24, 5)
    FIELD(PIO_SM_EXECCTRL, SIDE_PINDIR, 29, 1)
    FIELD(PIO_SM_EXECCTRL, SIDE_EN, 30, 1)
    FIELD(PIO_SM_EXECCTRL, EXEC_STALLED, 31, 1)

typedef struct RP2040PIOState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    
    MemoryRegion iomem;
    qemu_irq irq[2];  /* 2 IRQs per PIO block */
    
    /* Shared instruction memory - 32 instructions */
    uint16_t instr_mem[32];
    
    /* State machines - 4 per PIO */
    struct {
        bool enabled;
        uint32_t clkdiv;
        uint32_t execctrl;
        uint32_t shiftctrl;
        uint32_t addr;
        uint32_t instr;
        uint32_t pinctrl;
        
        /* State machine execution state */
        uint32_t pc;
        uint32_t x, y;
        uint32_t isr, osr;
        int isr_shift_count, osr_shift_count;
        
        /* FIFOs */
        struct {
            uint32_t data[8];
            int rptr, wptr;
            int level;
        } txfifo, rxfifo;
    } sm[4];
    
    /* IRQ state */
    uint32_t irq_ctrl;
    uint32_t irq_force;
    uint32_t irq_status;
} RP2040PIOState;

/* Example: DMA Controller Pattern */
#define TYPE_RP2040_DMA "rp2040-dma"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040DMAState, RP2040_DMA)

/* DMA Channel Registers */
REG32(DMA_CH_READ_ADDR, 0x00)
REG32(DMA_CH_WRITE_ADDR, 0x04)
REG32(DMA_CH_TRANS_COUNT, 0x08)
REG32(DMA_CH_CTRL_TRIG, 0x0c)
    FIELD(DMA_CH_CTRL_TRIG, EN, 0, 1)          /* Enable */
    FIELD(DMA_CH_CTRL_TRIG, HIGH_PRIORITY, 1, 1)
    FIELD(DMA_CH_CTRL_TRIG, DATA_SIZE, 2, 2)   /* 0=byte, 1=half, 2=word */
    FIELD(DMA_CH_CTRL_TRIG, INCR_READ, 4, 1)
    FIELD(DMA_CH_CTRL_TRIG, INCR_WRITE, 5, 1)
    FIELD(DMA_CH_CTRL_TRIG, RING_SIZE, 6, 4)
    FIELD(DMA_CH_CTRL_TRIG, RING_SEL, 10, 1)
    FIELD(DMA_CH_CTRL_TRIG, CHAIN_TO, 11, 4)
    FIELD(DMA_CH_CTRL_TRIG, TREQ_SEL, 15, 6)
    FIELD(DMA_CH_CTRL_TRIG, IRQ_QUIET, 21, 1)
    FIELD(DMA_CH_CTRL_TRIG, BSWAP, 22, 1)
    FIELD(DMA_CH_CTRL_TRIG, SNIFF_EN, 23, 1)
    FIELD(DMA_CH_CTRL_TRIG, BUSY, 24, 1)
    FIELD(DMA_CH_CTRL_TRIG, WRITE_ERROR, 29, 1)
    FIELD(DMA_CH_CTRL_TRIG, READ_ERROR, 30, 1)
    FIELD(DMA_CH_CTRL_TRIG, AHB_ERROR, 31, 1)

typedef struct RP2040DMAChannel {
    uint32_t read_addr;
    uint32_t write_addr;
    uint32_t transfer_count;
    uint32_t ctrl;
    
    /* Shadow registers for chaining */
    uint32_t al1_ctrl;
    uint32_t al1_read_addr;
    uint32_t al1_write_addr;
    uint32_t al1_transfer_count_trig;
    
    /* Internal state */
    bool busy;
    uint32_t transfers_remaining;
} RP2040DMAChannel;

typedef struct RP2040DMAState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    
    MemoryRegion iomem;
    qemu_irq irq[2];  /* 2 shared IRQs */
    
    /* 12 DMA channels */
    RP2040DMAChannel channels[12];
    
    /* Global registers */
    uint32_t intr;     /* Interrupt status */
    uint32_t inte0;    /* Interrupt enable for IRQ 0 */
    uint32_t inte1;    /* Interrupt enable for IRQ 1 */
    uint32_t ints0;    /* Interrupt status for IRQ 0 */
    uint32_t ints1;    /* Interrupt status for IRQ 1 */
    
    /* Timer for pacing */
    uint32_t timer[4];
    
    /* Debug */
    uint32_t chan_abort;
    uint32_t n_channels;
} RP2040DMAState;

/* Timer/Counter Pattern */
#define TYPE_RP2040_TIMER "rp2040-timer"
OBJECT_DECLARE_SIMPLE_TYPE(RP2040TimerState, RP2040_TIMER)

REG32(TIMER_TIMEHW, 0x00)    /* Write to bits 63:32 of time */
REG32(TIMER_TIMELW, 0x04)    /* Write to bits 31:0 of time */
REG32(TIMER_TIMEHR, 0x08)    /* Read bits 63:32 of time */
REG32(TIMER_TIMELR, 0x0c)    /* Read bits 31:0 of time */
REG32(TIMER_ALARM0, 0x10)    /* Alarm 0 */
REG32(TIMER_ALARM1, 0x14)    /* Alarm 1 */
REG32(TIMER_ALARM2, 0x18)    /* Alarm 2 */
REG32(TIMER_ALARM3, 0x1c)    /* Alarm 3 */
REG32(TIMER_ARMED, 0x20)     /* Indicates armed alarms */
REG32(TIMER_TIMERAWH, 0x24)  /* Raw read bits 63:32 */
REG32(TIMER_TIMERAWL, 0x28)  /* Raw read bits 31:0 */
REG32(TIMER_DBGPAUSE, 0x2c)  /* Debug pause */
REG32(TIMER_PAUSE, 0x30)     /* Pause timer */
REG32(TIMER_INTR, 0x34)      /* Interrupt status */
REG32(TIMER_INTE, 0x38)      /* Interrupt enable */
REG32(TIMER_INTF, 0x3c)      /* Interrupt force */
REG32(TIMER_INTS, 0x40)      /* Interrupt status after enable */

typedef struct RP2040TimerState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/
    
    MemoryRegion iomem;
    qemu_irq irq[4];  /* 4 alarm IRQs */
    
    /* Timer state */
    QEMUTimer *timer;
    uint64_t tick_offset;
    
    /* Registers */
    uint32_t alarm[4];
    uint32_t armed;
    uint32_t pause;
    uint32_t inte;
    uint32_t intf;
    
    /* Debug */
    uint32_t dbgpause;
} RP2040TimerState;

/* Common patterns for all peripherals */

/* 1. Read/Write operations template */
static inline uint64_t rp2040_peripheral_read(void *opaque, hwaddr offset,
                                              unsigned size)
{
    /* Check alignment */
    if (offset & 3) {
        qemu_log_mask(LOG_GUEST_ERROR, 
                     "%s: Unaligned read at offset 0x%" HWADDR_PRIx "\n",
                     __func__, offset);
        return 0;
    }
    
    /* Handle specific registers */
    switch (offset) {
    case REG_OFFSET:
        return s->reg_value;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                     "%s: Bad offset 0x%" HWADDR_PRIx "\n", 
                     __func__, offset);
        return 0;
    }
}

/* 2. Register write with set/clear/xor pattern */
static inline void rp2040_gpio_write_atomic(RP2040GPIOState *s, 
                                            hwaddr offset, uint32_t value)
{
    switch (offset) {
    case A_GPIO_OUT:
        s->out_value = value;
        break;
    case A_GPIO_OUT_SET:
        s->out_value |= value;
        break;
    case A_GPIO_OUT_CLR:
        s->out_value &= ~value;
        break;
    case A_GPIO_OUT_XOR:
        s->out_value ^= value;
        break;
    }
    /* Update actual GPIO pins */
    rp2040_gpio_update_pins(s);
}

/* 3. FIFO implementation pattern */
static inline bool fifo_is_full(const struct fifo *f)
{
    return ((f->wptr + 1) & (FIFO_SIZE - 1)) == f->rptr;
}

static inline bool fifo_is_empty(const struct fifo *f)
{
    return f->wptr == f->rptr;
}

static inline void fifo_push(struct fifo *f, uint32_t data)
{
    if (!fifo_is_full(f)) {
        f->data[f->wptr] = data;
        f->wptr = (f->wptr + 1) & (FIFO_SIZE - 1);
    }
}

static inline uint32_t fifo_pop(struct fifo *f)
{
    uint32_t data = 0;
    if (!fifo_is_empty(f)) {
        data = f->data[f->rptr];
        f->rptr = (f->rptr + 1) & (FIFO_SIZE - 1);
    }
    return data;
}

/* 4. IRQ handling pattern */
static inline void rp2040_update_irq(SysBusDevice *sbd, 
                                    qemu_irq *irqs, int n_irqs,
                                    uint32_t status, uint32_t enable)
{
    int i;
    for (i = 0; i < n_irqs; i++) {
        qemu_set_irq(irqs[i], (status & enable) & (1 << i));
    }
}

/* 5. Reset pattern */
static inline void rp2040_peripheral_reset(DeviceState *dev)
{
    /* Reset all registers to default values */
    /* Clear FIFOs */
    /* Deassert all IRQs */
}

/* 6. VMState pattern for migration */
#define VMSTATE_RP2040_FIFO(_field, _state) \
    VMSTATE_STRUCT(_field, _state, 0, vmstate_rp2040_fifo, struct fifo)

static const VMStateDescription vmstate_rp2040_fifo = {
    .name = "rp2040_fifo",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(data, struct fifo, FIFO_SIZE),
        VMSTATE_INT32(rptr, struct fifo),
        VMSTATE_INT32(wptr, struct fifo),
        VMSTATE_END_OF_LIST()
    }
};

#endif /* HW_ARM_RP2040_PERIPHERALS_H */