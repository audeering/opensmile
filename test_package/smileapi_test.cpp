#include <iostream>
#include <smileapi/SMILEapi.h>

int main()
{
    std::cout << "Create smileobj .." << std::endl;
    smileobj_t *smileobj = smile_new();

    std::cout << "Free smileobj .." << std::endl;
    smile_free(smileobj);
}
