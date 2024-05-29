class Potato {
public:
    int hops;
    int size;
    int trace[512];
    Potato() : hops(0), size(0) { }
    void print_traces() {
        std::cout << "Trace of potato:" << std::endl;
        for (int i = 0; i < size; i++) {
            std::cout << trace[i];
            if (i < size - 1) {
                std::cout << ",";
            }
        }
        std::cout << std::endl;
    }
};

