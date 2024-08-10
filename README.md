# pyxi

```cpp
#include <cstdint>
#include <string>
#include <vector>

#include "pyxi.hpp"

struct Person
{
    std::string name;
    uint32_t id;
};

class Group
{
public:
    void add(const std::string& name)
    {
        people_.push_back({ name, nextId_++ });
    }

    void serialize(pyxi::Serializer& ser)
    {
        ser << people_;
    }

    void deserialize(pyxi::deserialize& des)
    {
        des >> people_;
        nextId_ = people_.back().id + 1;
    }
private:
    std::vector<Person> people_;
    uint32_t nextId_ = 0;
}

int main()
{
    Group group;
    group.add("Guy");
    group.add("Man");

    std::vector<uint8_t> bytes = pyxi::align(pyxi::serialize(group), 8);

    // bytes
    // =======================
    // 00 00 00 00 00 00 00 02 // people_.size()         == 2
    // 00 00 00 00 00 00 00 03 // people_[0].name.size() == 3
    // 47 75 79                // people_[0].name        == "Guy"
    //          00 00 00 00 00
    // 00 00 00                // people_[0].id          == 0
    //          00 00 00 00 00
    // 00 00 03                // people_[1].name.size() == 3
    //          4D 61 6E       // people_[1].name        == "Man"
    //                   00 00
    // 00 00 00 00 00 01       // people_[1].id          == 1
    //                   00 00 // align(..., 8)
}
```

## FAQ

### Where are the serialize and deserialize functions for `Person`?

There aren't any. For standard layout objects like `Person`, the members can
be inferred and handled like any other type. This works with nesting too,
meaning you could have a vector of some other custom type as well.

### Hang on, 8 bytes for the size of collections?

Currently, yes. This is because `size_t` is used to ensure the size fits.
Don't worry, I'm planning on adding a utility object like `Bits` and `Spare`
to handle custom size types.
