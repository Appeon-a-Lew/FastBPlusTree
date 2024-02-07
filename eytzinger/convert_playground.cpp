#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <stdint.h>
#include <vector>
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
struct PageSlot
{
    u16 offset;
    u8 headLen;
    u8 remainderLen;
    union
    {
        u32 head;
        u8 headBytes[4];
    };
};

bool operator<(const PageSlot &a, const PageSlot &b)
{
    return a.head < b.head;
}
// Assume convertToEytzinger and convertFromEytzinger functions are defined here

void generateRandomSortedArray(PageSlot *array, int n)
{
    std::srand(std::time(nullptr));
    for (int i = 0; i < n; ++i)
    {
        array[i].head = std::rand() % 10000; // Random key between 0 and 99
    }
    std::sort(array, array + n);
}

bool isSorted(const PageSlot *array, int n)
{
    for (int i = 1; i < n; ++i)
    {
        if (array[i] < array[i - 1])
        {
            return false;
        }
    }
    return true;
}

bool isEytzingerLayout(const PageSlot *array, int index, int n)
{
    int leftChildIndex = 2 * index ;
    int rightChildIndex = 2 * index + 1;
    if (leftChildIndex >= n)
        return true;
    if (rightChildIndex >= n && array[leftChildIndex].head < array[index].head)
        return true;

    // Check if left child exists and if its key is less than or equal to the parent's key
    if (leftChildIndex < n && array[leftChildIndex].head > array[index].head)
    {
        std::cout << "The array is wrong at " << index << "and  the left is " << leftChildIndex << std::endl;
        return false;
    }

    // Check if right child exists and if its key is greater than or equal to the parent's key
    if (rightChildIndex < n && array[rightChildIndex].head < array[index].head)
    {
        std::cout << "The array is wrong at " << index << "and  the right is " << rightChildIndex << std::endl;

        return false;
    }

    // Recursively check for children nodes if they exist
    return isEytzingerLayout(array, leftChildIndex, n) &&
           isEytzingerLayout(array, rightChildIndex, n);
}

// Wrapper function to start from the root
bool checkEytzingerLayout(const PageSlot *array, int n)
{
    if (n == 0)
        return true; // Empty array is trivially in Eytzinger layout
    return isEytzingerLayout(array, 1, n);
}

int eytzinger(const PageSlot *sortedArray, PageSlot *eytzingerArray, int n, int i = 0, int k = 1)
{
    if (k <= n)
    {
        i = eytzinger(sortedArray, eytzingerArray, n, i, 2 * k);
        eytzingerArray[k] = sortedArray[i++];
        i = eytzinger(sortedArray, eytzingerArray, n, i, 2 * k + 1);
    }
    return i;
}


// Wrapper function
void convertToEytzinger(PageSlot *sortedArray, int n)
{
    PageSlot *eytzingerArray = new PageSlot[n + 1];
    // int index = 0;
    // toEytzingerLayout(sortedArray, eytzingerArray, 0, n - 1, index);
    eytzinger(sortedArray, eytzingerArray, n);
    std::copy(eytzingerArray, eytzingerArray + n + 1, sortedArray);
    delete[] eytzingerArray;
}

void fromEytzingerLayout(const PageSlot *eytzingerArray, PageSlot *sortedArray, int index, int &sortedIndex, int n)
{
    if (index >= n)
    {
        return;
    }
    fromEytzingerLayout(eytzingerArray, sortedArray, 2 * index , sortedIndex, n);
    sortedArray[sortedIndex++] = eytzingerArray[index];
    fromEytzingerLayout(eytzingerArray, sortedArray, 2 * index +1, sortedIndex, n);
}

// Wrapper function
void convertFromEytzinger(PageSlot *eytzingerArray, int n)
{
    PageSlot *sortedArray = new PageSlot[n];
    int sortedIndex = 0;
    fromEytzingerLayout(eytzingerArray, sortedArray, 1, sortedIndex, n+1);
    std::copy(sortedArray, sortedArray + n, eytzingerArray);
    // eytzingerArray[n] = {0,0,0,{0}};
    delete[] sortedArray;
}
void loadFromVector(std::vector<long> x, PageSlot* array){
    for(size_t i = 0 ; i < x.size(); i++)
    {
        array[i].head = x.at(i);
    }
}

int main()
{
   
    std::vector<long> x = {3540058112, 3138060288, 4077715456, 101384192, 3054370816, 3858759680, 4010475520, 17301504, 50331648, 755564544, 872677376, 989921280, 1292763136, 1359347712, 2013724672, 2030174208, 2164654080, 2215378944, 2215510016, 2432696320, 3154771968, 2936340480, 1108213760, 1946943488, 2097414144, 2181234688, 2215182336, 2298806272, 2315386880, 2416508928, 2550136832, 2685140992, 2819096576, 2819227648, 2986606592, 3539992576, 3775791104, 4278583296, 2365587456, 2399272960, 990314496, 2751594496, 3472949248, 2885746688, 3993632768, 4110811136, 2869166080, 83951616, 906559488, 1292238848, 1795948544, 1980628992, 2500198400, 2550857728, 2634022912, 2651324416, 2769027072, 3272409088, 3322740736, 4060151808, 3489988608, 3389128704, 1208221696, 1426587648, 1426915328, 1812135936, 2046820352, 2869100544, 3825532928, 2047410176, 3355639808, 3355770880, 3355901952, 3356033024, 3356098560, 3356164096, 3356229632, 3356360704, 3372220416, 3372351488, 3372482560, 3372548096, 3372679168, 3372744704, 3372810240, 3372875776, 3372941312, 3373006848, 3373137920, 3389063168, 3389128704, 3389325312, 3389521920, 3389652992, 3389718528, 3405840384, 3405905920, 3406168064, 3406364672, 3406626816, 3406692352, 3422552064, 3422814208, 3422879744, 3423010816, 3423076352, 3423272960, 3423404032, 3423469568, 3439329280, 3439394816, 3439525888, 3439656960, 3439788032, 3439853568, 3439984640, 3440050176, 3440181248, 3440246784, 3456172032, 3456368640, 3456434176, 3456630784, 3456827392, 3472949248, 3473014784, 3473080320, 3473211392, 3473342464, 3473473536, 3473604608, 3473670144, 3489660928, 3489726464, 3489792000, 3489923072, 3490054144, 3490119680, 3490447360, 3490512896, 3490578432, 3506438144, 3506569216, 3506700288, 3506831360, 3507027968, 3507093504, 3507159040, 3507224576, 3507290112, 3523280896, 3523346432, 3523477504, 3523543040, 3523608576, 3523870720, 3524001792, 3524132864, 3540189184, 3540451328, 3540516864, 3540647936, 3540713472, 3540844544, 3540910080, 3556835328, 3556900864, 3556966400, 3557031936, 3557294080, 3557359616};
    int n = x.size(); // Change this for different array sizes
    int printsize = 2*n+1;
    PageSlot *array = new PageSlot[printsize];

    // Generate a random sorted array
    // generateRandomSortedArray(array, n);
    loadFromVector(x, array);
    std::cout << "Original sorted array:\n";
    for (int i = 0; i < printsize; ++i)
    {
        std::cout << array[i].head << " ";
    }
    std::cout << "\n";

    // Convert to Eytzinger layout
    convertToEytzinger(array, n);
    std::cout << "Eytzinger layout:\n";
    for (int i = 0; i < printsize; ++i)
    {
        std::cout << array[i].head << ", ";
    }
    std::cout << "\n";

    if (checkEytzingerLayout(array, n))
    {
        std::cout << "Array is in Eytzinger layout." << std::endl;
    }
    else
    {
        std::cout << "Array is not in Eytzinger layout." << std::endl;
    }

    // Convert back to sorted array
    convertFromEytzinger(array, n);
    std::cout << "Sorted array after conversion:\n";
    for (int i = 0; i < printsize; ++i)
    {
        std::cout << array[i].head << " ";
    }
    std::cout << "\n";

    // Check if the array is sorted
    if (isSorted(array, n))
    {
        std::cout << "The array is correctly sorted.\n";
    }
    else
    {
        std::cout << "The array is not sorted correctly.\n";
    }

    delete[] array;
    return 0;
}
