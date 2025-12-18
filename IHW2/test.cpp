#include <iostream>
#include <cmath>

// Прямое вычисление по степенному ряду (ограничено 20 членами)
double series_ln(double x) {
    double sum = 0.0;
    double power = x;
    for (int n = 1; n <= 20; ++n) {
        sum -= power / n;
        power *= x;
    }
    return sum;
}

int main() {
    const double t1 = 0.1;
    const double t2 = 0.5;
    const double t3 = -0.3;

    std::cout << std::fixed;

    std::cout << "x = 0.1  ln(1-0.1) (series) = " << series_ln(t1) << '\n';
    std::cout << "x = 0.5  ln(1-0.5) (series) = " << series_ln(t2) << '\n';
    std::cout << "x = -0.3 ln(1-(-0.3)) (series) = " << series_ln(t3) << '\n';

    std::cout << "\nТе же значения через std::log:\n";
    std::cout << "x = 0.1  ln(1-0.1) (std::log) = " << std::log(1 - t1) << '\n';
    std::cout << "x = 0.5  ln(1-0.5) (std::log) = " << std::log(1 - t2) << '\n';
    std::cout << "x = -0.3 ln(1-(-0.3)) (std::log) = " << std::log(1 - t3) << '\n';

    return 0;
}
