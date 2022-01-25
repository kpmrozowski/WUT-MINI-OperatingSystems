struct library {
    int some_value;
    void (*method1)(void);
    void (*method2)(int);
    /* ... */
};

extern struct library Library;
