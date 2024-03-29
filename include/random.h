#pragma once

#include <random>


class Random
{
    static std::mt19937 *_rng;

public:
    static void init();
    static int get(int max);
    static int get(int min, int max);
    static double getDouble(double max = 1.0);
    static double getDouble(double min, double max);
};