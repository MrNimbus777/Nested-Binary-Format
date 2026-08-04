# Nested Binary Format (NBF)

NBF is a lightweight library for serializing hierarchical typed data into a compact binary representation.

NBF provides:

- nested object structures
- typed homogeneous arrays
- raw binary blobs
- strings
- signed and unsigned integers
- floating-point values
- recursive encoding and decoding
- explicit memory ownership

---
# C (nested_binary_format.h)

## Installation

Include the header in one translation unit with `NBF_IMPLEMENTATION` defined:

```c
#define NBF_IMPLEMENTATION
#include "nested_binary_format.h"
```

In other files:

```c
#include "nested_binary_format.h"
```

You may also want to strip the names, then the inclusion should look like this:
```c
#define NBF_IMPLEMENTATION
#define NBF_STRIP_PREFIXES
#include "nested_binary_format.h"
```

---

## Basic Example

```c
#define NBF_STRIP_PREFIXES
#define NBF_IMPLEMENTATION
#include "nested_binary_format.h"

int main(){
    value_t value = VSTACK_NODE(
        VFIELD("printf", VSTRING("HELLO WORLD"))
    );
    printf("To encode: "); value_print(&value); printf("\n");
    
    size_t buffer_size = value_sizeof(&value);
    byte buffer[buffer_size];

    value_encode(&value, buffer);

    printf("Encoded bytes: "); printf_bytes(buffer, buffer_size); printf("\n");

    byte* ptr = buffer;
    value_t decoded = value_decode(&ptr);

    printf("Decoded: "); value_print(&decoded); printf("\n");

    value_free(&decoded);
    return 0;
}
```

Output:

```txt
To encode: {"printf": "HELLO WORLD"}
Encoded bytes: 01 00 01 00 06 70 72 69 6E 74 66 04 00 00 00 0B 48 45 4C 4C 4F 20 57 4F 52 4C 44
Decoded: {"printf": "HELLO WORLD"}
```
---
# Java (nested_binary_format.jar)

## Instalation

Simply download the `nested_binary_format.jar/nested_binary_format.jar` in your working directory following the right steps for installing a local lib for your Java project manager (Gradle, Maven...)

---

## Basic Example

```java
...
import net.nimbus.nbf.NestedBinaryFormat;
import net.nimbus.nbf.value.Value;

import static net.nimbus.nbf.value.Value.*;

public class Main {
    public static void main(String[] args) {

        Value value = node()
                .put("printf", "HELLO WORLD");

        System.out.println("To encode: " + value);

        byte[] encoded = NestedBinaryFormat.encode(value);

        System.out.println("Encoded bytes:");
        System.out.println(NestedBinaryFormat.toHex(encoded));

        Value decoded = NestedBinaryFormat.decode(encoded);

        System.out.println("Decoded: " + decoded);
    }
}
```

Output:

```txt
To encode:  Object { printf: "HELLO WORLD" }
Encoded bytes:
01 00 01 00 06 70 72 69 6E 74 66 04 00 00 00 0B 48 45 4C 4C 4F 20 57 4F 52 4C 44
Decoded:  Object { printf: "HELLO WORLD" }
```

---
# JavaScript (nested_binary_format.js)

## Instalation

Obviously is as simple as for the C version. 
Just dowload the `nested_binary_format.js` in your working directory and write at the begin of your script `import { NBF } from "/nested_binary_format.js";`. Don't forget to mark your script with `type="module"`

---

## Basic Example

```html
<script type="module">
import { NBF } from "/nested_binary_format.js";

const value = {printf: "HELLO WORLD"};
console.log("To encode: ", value);

const encoded = NBF.encode(value);

console.log("Encoded bytes: "); NBF.printBytes(encoded);

const decoded = NBF.decode(encoded);

console.log("Decoded: ", decoded); 
</script>
```

Browser's Console:

```txt
To encode:  Object { printf: "HELLO WORLD" }
Encoded bytes:
01 00 01 00 06 70 72 69 6E 74 66 04 00 00 00 0B 48 45 4C 4C 4F 20 57 4F 52 4C 44
Decoded:  Object { printf: "HELLO WORLD" }
```

---

### Future plans:
- To add files related functionality (read, write)
- To implement partial decoding
- To integrate a compression algorithm for the binary data
- To translate this lib into other programming languages (e.g. C++, Java, Go, Rust, Python)
- To conquer the world.
