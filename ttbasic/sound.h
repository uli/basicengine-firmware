class BasicSound {
public:
    static void begin(void);
    static void playMml(const char *data);
    static void stopMml();
    static void pumpEvents();
private:
    static void defaults();
};

extern BasicSound sound;
