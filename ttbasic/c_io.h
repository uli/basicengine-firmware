int c_gpio_set_pin(int portno, int pinno, int data);
int c_gpio_get_pin(int portno, int pinno);
int c_gpio_set_pin_mode(int portno, int pinno, int mode);
int eb_i2c_write(unsigned char addr, const char *data, int count);
int eb_i2c_read(unsigned char addr, char *data, int count);
