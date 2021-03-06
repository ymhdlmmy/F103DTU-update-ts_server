#include "bsp_rtc.h"
#include "wdz_rtc_time.h"

/* 秒中断标志，进入秒中断时置1，当时间被刷新之后清0 */
__IO uint32_t TimeDisplay = 1;
extern u8 rtcbuffer[];



/*
 * 函数名：NVIC_Configuration
 * 描述  ：配置RTC秒中断的主中断优先级为1，次优先级为0
 * 输入  ：无
 * 输出  ：无
 * 调用  ：外部调用
 （当前没用这个中断功能）
 */
void RTC_NVIC_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Configure one bit for preemption priority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);

    /* Enable the RTC Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}


/*
 * 函数名：RTC_CheckAndConfig
 * 描述  ：检查并配置RTC
 * 输入  ：用于读取RTC时间的结构体指针
 * 输出  ：无
 * 调用  ：外部调用
 */
void RTC_CheckAndConfig( )
{
    //***********调试使用

    //RTC_Configuration();
    //	Time_Adjust(tm);  //输入当前时间，修改计算RTC计数值
    //	BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
    //***************调试使用

    /*在启动时检查备份寄存器BKP_DR1，如果内容不是0xA5A5,
    则需重新配置时间并询问用户调整时间*/
    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)
    {
        //如果想在程序运行中修改RTC的计数值，必须要开启备份域的写允许，千万要记住，下面的两句 2014.9.20 WDZ
        /* Enable PWR and BKP clocks */
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
        /* Allow access to BKP Domain */
        PWR_BackupAccessCmd(ENABLE);


        /* RTC Configuration */
        RTC_Configuration();
        /* Adjust time by users typed on the hyperterminal */
        RTC_WaitForLastTask();
        my_EEPROM_TO_RTC(rtcbuffer,0); //读取EEPROM中的时标，修改计算RTC计数值******************
        RTC_WaitForLastTask();

        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
    }
    else
    {
        //如果想在程序运行中修改RTC的计数值，必须要开启备份域的写允许，千万要记住，下面的两句 2014.9.20 WDZ
        /* Enable PWR and BKP clocks */
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
        /* Allow access to BKP Domain */
        PWR_BackupAccessCmd(ENABLE);

        /*启动无需设置新时钟*/
        /*检查是否掉电重启*/
        if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
        {

        }
        /*检查是否Reset复位*/
        else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
        {

        }

        //printf("\r\n No need to configure RTC....");
        /* RTC Configuration */
        RTC_Configuration();

        /*等待寄存器同步*/
        RTC_WaitForSynchro();

        /*允许RTC秒中断*/
        RTC_ITConfig(RTC_IT_SEC, ENABLE);
        //RTC_ITConfig(RTC_IT_SEC, DISABLE);

        /*等待上次RTC寄存器写操作完成*/
        RTC_WaitForLastTask();
        /*读取当前RTC的值写入到EEPROM中  重新启动后，RTC没有继续计数，从新开始计数，RTC时间乱了*/
        //my_RTC_TO_EEPROM(rtcbuffer,0);
        //读取EEPROM中的时标，修改计算RTC计数值******************
        my_EEPROM_TO_RTC(rtcbuffer,0);
        RTC_WaitForLastTask();


    }
    /* Clear reset flags */
    RCC_ClearFlag();

}




/*
 * 函数名：RTC_Configuration
 * 描述  ：配置RTC
 * 输入  ：无
 * 输出  ：无
 * 调用  ：外部调用
 */
void RTC_Configuration(void)
{
    int ii=0;
    int my_rcc_status=0;
    int my_status=0;
    /* Enable PWR and BKP clocks */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

    /* Allow access to BKP Domain */
    PWR_BackupAccessCmd(ENABLE);

    /* Reset Backup Domain */
    BKP_DeInit();

    /* Enable LSE  配置使用外部低速时钟32768HZ 旧板子1批次的外部低速时钟有问题，不能用，只能用内部低速时钟LSI*/
    RCC_LSEConfig(RCC_LSE_ON);
    /* Wait till LSE is ready */
    my_rcc_status=RCC_GetFlagStatus(RCC_FLAG_LSERDY);
    while (ii<20 && my_rcc_status==RESET)
    {
        Delay_us(50);
        my_rcc_status=RCC_GetFlagStatus(RCC_FLAG_LSERDY);
        ii++;
    }
    if(ii<20)  //硬件启动成功
    {
        my_status=0;
    }
    else if(ii>=20) //硬件启动失败
        my_status=1;


    if(my_status==0)
    {
        /* Select LSE as RTC Clock Source */
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        //USART_printf(&huart3,"LSE外部低频时钟\r\n");  //@@调试使用
    }

    else if(my_status==1)
    {
        RCC_LSEConfig(RCC_LSE_OFF);  //关闭外部低速时钟
        /* Enable LSI 配置使用内部低速时钟*/
        RCC_LSICmd(ENABLE);
        /* Wait till LSI is ready */
        while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)	{}
        /* Select LSI as RTC Clock Source */
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        //USART_printf(&huart3,"LSI内部低频时钟\r\n");  //@@调试使用
    }

    //**********************************************************************
    /* Enable RTC Clock */
    RCC_RTCCLKCmd(ENABLE);

    /* Wait for RTC registers synchronization
     * 因为RTC时钟是低速的，内环时钟是高速的，所以要同步
     */
    RTC_WaitForSynchro();

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();

    /* Enable the RTC Second */
    RTC_ITConfig(RTC_IT_SEC, ENABLE);
    //RTC_ITConfig(RTC_IT_SEC, DISABLE);

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();

    /* Set RTC prescaler: set RTC period to 1sec */
    if(my_status==0)  //解决RTC时钟过慢的问题，原来用外部时钟，但用内部时钟算的，错了，时钟慢了
        RTC_SetPrescaler(32767); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) = 1HZ */
    else
        RTC_SetPrescaler(40000-1); /* RTC period = RTCCLK/RTC_PR = (40KHz)/(32767+1) = 1HZ */

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
}




/*
 * 函数名：Time_Adjust
 * 描述  ：时间调节
 * 输入  ：用于读取RTC时间的结构体指针
 * 输出  ：无
 * 调用  ：外部调用
 */
void Time_Adjust(struct rtc_time *tm)
{
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();

    /* Get time entred by the user on the hyperterminal */
    Time_Regulate(tm);  //输入当前的时间，年，月，日，小时，分钟，秒，放到结构体中

    /* Get wday */
    // GregorianDay(tm);   //利用结构体的数据计算出当前的星期几？

    /* 修改当前RTC计数寄存器内容 */
    RTC_SetCounter(mktimev(tm));  //  修改计数值，这个很重要，计算从1970年算起的计算数值，mktimev()函数进行计数值的计算

    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
}

/*
 * 函数名：Time_Regulate
 * 描述  ：返回用户在超级终端中输入的时间值，并将值储存在
 *         RTC 计数寄存器中。
 * 输入  ：用于读取RTC时间的结构体指针
 * 输出  ：用户在超级终端中输入的时间值，单位为 s
 * 调用  ：内部调用
 */
extern int T_year,T_month,T_day,T_hh,T_mm,T_ss;
void Time_Regulate(struct rtc_time *tm)
{
    tm->tm_year=T_year;
    tm->tm_mon=T_month;
    tm->tm_mday=T_day;
    tm->tm_hour=T_hh;
    tm->tm_min=T_mm;
    tm->tm_sec=T_ss;

}



/*
 * 函数名：Time_Show
 * 描述  ：在超级终端中显示当前时间值
 * 输入  ：无
 * 输出  ：无
 * 调用  ：外部调用
 */
void Time_Show(void)
{   struct rtc_time tm;
    uint32_t TimeVar=0;

    if (TimeDisplay == 1)
    {
        /* Display current time */
        TimeVar=RTC_GetCounter();
        Time_Display( TimeVar,&tm); 		 //利用RTC函数获得当前的计数值RTC_GetCounter()
        TimeDisplay = 1;
    }

}
/*
 * 函数名：Time_Display
 * 描述  ：显示当前时间值
 * 输入  ：-TimeVar RTC计数值，单位为 s
 * 输出  ：无
 * 调用  ：内部调用
 */
void Time_Display(uint32_t TimeVar,struct rtc_time *tm)
{
//	   static uint32_t FirstDisplay = 1;
    uint32_t BJ_TimeVar;
//	   uint8_t str[15]; // 字符串暂存

    /*  把标准时间转换为北京时间*/
    //BJ_TimeVar =TimeVar + 8*60*60;

    BJ_TimeVar =TimeVar;  //不算时区问题，就认为当前是 基时（格林尼治时间）

    to_tm(BJ_TimeVar, tm);/*把定时器的值转换为北京时间,存在tm里*/


    /* 输出时间戳，公历时间，利用串口1进行输出 */
    // printf(" UNIX time = %d  now time: %d-%d-%d(week-%d)  %0.2d:%0.2d:%0.2d \r\n",
    //TimeVar,tm->tm_year,tm->tm_mon,tm->tm_mday,tm->tm_wday,tm->tm_hour,tm->tm_min,tm->tm_sec);


    //输出时间戳，公历时间
    USART_printf(&huart3,"\r\n TIME-%d=%d-%d-%d  %d:%d:%d",TimeVar,tm->tm_year,tm->tm_mon,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);



}




/************************END OF FILE***************************************/
