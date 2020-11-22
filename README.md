# AzureGUIX

改软件包目前只有单色屏，RGB屏需自行修改驱动接口。请阅读pdf寻找修改方式

开发者使用时需自行更改Examples中MainTask.c中的下列参数

```
extern ULONG display_1_canvas_memory[512]; 
此处数组大小应与Studio生成的大小相同

uint8_t oled_canvas_memory[2048];
此处大小与上面数组大小相同  目的是为了存储转换扫描方式后的数据
实际所用屏幕可能和Studio中的界面扫描方式不同 （Studio中界面扫描方式为 从左到右，从上到下）


自行实现oled打点函数
void GuiDrawPoint(uint8_t x, uint8_t y, uint8_t color)
{
    .....
}

在外部文件中自行实现下列函数  
extern void lcd_address(uint8_t page, uint8_t column);
extern void transfer_data(uint8_t *data, uint8_t len);

```



使用GUIX Studio可打开GUIX Studio OLED Demo中的工程
GUIX Studio生成的文件需要替换Example文件夹中的同名文件才能生效
