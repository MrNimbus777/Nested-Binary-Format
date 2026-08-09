package nbf

/*
#cgo CFLAGS: -I${SRCDIR}/include
#cgo linux LDFLAGS: -L${SRCDIR}/../lib/linux -lnbf
#cgo darwin LDFLAGS: -L${SRCDIR}/../lib/darwin -lnbf
#cgo windows LDFLAGS: -L${SRCDIR}/../lib/windows -lnbf

#include "nested_binary_format.h"
#include <string.h>
*/
import "C"
import (
	"errors"
	"runtime"
	"unsafe"
)

type Type uint8

const (
	TypeEmpty Type = Type(C.NBF_TYPES_EMPTY)

	TypeNode Type = Type(C.NBF_TYPES_NODE)
	TypeList Type = Type(C.NBF_TYPES_LIST)

	TypeRaw    Type = Type(C.NBF_TYPES_RAW)
	TypeString Type = Type(C.NBF_TYPES_STRING)

	TypeInt8   Type = Type(C.NBF_TYPES_INT8)
	TypeInt16  Type = Type(C.NBF_TYPES_INT16)
	TypeInt32  Type = Type(C.NBF_TYPES_INT32)
	TypeInt64  Type = Type(C.NBF_TYPES_INT64)
	TypeUint8  Type = Type(C.NBF_TYPES_UINT8)
	TypeUint16 Type = Type(C.NBF_TYPES_UINT16)
	TypeUint32 Type = Type(C.NBF_TYPES_UINT32)
	TypeUint64 Type = Type(C.NBF_TYPES_UINT64)

	TypeFloat32 Type = Type(C.NBF_TYPES_FLOAT32)
	TypeFloat64 Type = Type(C.NBF_TYPES_FLOAT64)
)

type Value struct {
	ptr C.nbf_value_t
}

type Node Value

type List struct {
	ptr *C.nbf_list_t
}

type Field struct {
	ptr *C.nbf_field_t
}

func (v *Value) Size() uint {
	return uint(C.nbf_value_sizeof(&v.ptr))
}

func Decode(buffer []byte) *Value {
	cursor := (*C.byte)(unsafe.Pointer(&buffer[0]))

	v := &Value{
		ptr: C.nbf_value_decode(&cursor),
	}
	runtime.SetFinalizer(v, (*Value).free_)

	return v
}

func (v *Value) Encode() ([]byte, uint) {
	size := v.Size()
	buffer := make([]byte, size)
	C.nbf_value_encode(&v.ptr, (*C.byte)(unsafe.Pointer(&buffer[0])))
	return buffer, size
}

func (v *Value) Type() Type {
	return Type(v.ptr._type)
}

func (v *Value) IsEmpty() bool {
	return v.Type() == TypeEmpty
}

func (v *Value) Clone() *Value {
	clone := &Value{
		ptr: C.nbf_value_clone(&v.ptr),
	}
	runtime.SetFinalizer(clone, (*Value).free_)

	return clone
}

func strviewcmp(a *C.char, alen uint, b string) int {
	blen := uint(len(b))

	n := alen
	if blen < n {
		n = blen
	}

	ap := unsafe.Pointer(a)
	bp := unsafe.Pointer(unsafe.StringData(b))

	for i := uint(0); i < n; i++ {
		av := *(*byte)(unsafe.Add(ap, uintptr(i)))
		bv := *(*byte)(unsafe.Add(bp, uintptr(i)))

		if av < bv {
			return -1
		}
		if av > bv {
			return 1
		}
	}

	if alen < blen {
		return -1
	}
	if alen > blen {
		return 1
	}
	return 0
}

func (n *Node) node() *C.nbf_node_t {
	return (*C.nbf_node_t)(unsafe.Pointer(&n.ptr.anon0[0]))
}

func (n *Node) Get(name string) *Field {
	var current *Field = &Field{ptr: nil}

	for {
		current = n.Next(current)
		if current == nil {
			break
		}
		if strviewcmp(current.ptr.name, uint(C.strlen(current.ptr.name)), name) == 0 {
			return current
		}
	}

	return nil
}

func (n *Node) Set(name string, value Value) (*Field, error) {
	cstr, err := toCStr(name)
	if err != nil {
		return nil, err
	}

	cfield := C.nbf_node_put(&n.ptr, cstr, value.ptr)

	if cfield == nil {
		C.free(unsafe.Pointer(cstr))
		return nil, errors.New("Failed to set the value")
	}
	cfield.__name_ownership = C.NBF_OWNERSHIP_HEAP

	return &Field{ptr: cfield}, nil
}
func (n *Node) Remove(name string) bool {
	cstr := C.CString(name)

	result := C.nbf_node_remove(&n.ptr, cstr) == 0

	C.free(unsafe.Pointer(cstr))

	return result
}
func (n *Node) Clear() {
	C.nbf_node_clear(&n.ptr)
}

func (n *Node) Size() uint16 {
	return uint16(C.nbf_node_size(n.node()))
}

func (n *Node) Next(current *Field) *Field {
	next := C.nbf_node_next(n.node(), current.ptr)

	if next == nil {
		return nil
	}

	return &Field{ptr: next}
}

func (n *Node) ForEach(fn func(*Field)) {
	var current *Field = &Field{ptr: nil}

	for {
		current = n.Next(current)
		if current == nil {
			return
		}

		fn(current)
	}
}

func (f *Field) Name() string {
	return C.GoString(f.ptr.name)
}
func (f *Field) Value() Value {
	return Value{ptr: f.ptr.value}
}
func (f *Field) Release() bool {
	return C.nbf_field_release(f.ptr) == 0
}

func (v *Value) tv_() *C.nbf_typeless_value_t {
	return (*C.nbf_typeless_value_t)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsNode() Node {
	return Node(*v)
}

func (v *Value) AsList() List {
	return List{
		ptr: (*C.nbf_list_t)(unsafe.Pointer(&v.ptr.anon0[0])),
	}
}

func (v *Value) AsRaw() []byte {
	raw := (*C.nbf_raw_t)(
		unsafe.Pointer(&v.ptr.anon0[0]),
	)

	return unsafe.Slice(
		(*byte)(unsafe.Pointer(raw.data)),
		raw.size,
	)
}

func (v *Value) AsString() string {
	cstr := *(*(*C.char))(unsafe.Pointer(&v.ptr.anon0[0]))
	return C.GoString(cstr)
}

func (v *Value) AsInt8() int8 {
	return *(*int8)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsInt16() int16 {
	return *(*int16)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsInt32() int32 {
	return *(*int32)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsInt64() int64 {
	return *(*int64)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsUint8() uint8 {
	return *(*uint8)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsUint16() uint16 {
	return *(*uint16)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsUint32() uint32 {
	return *(*uint32)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsUint64() uint64 {
	return *(*uint64)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsFloat32() float32 {
	return *(*float32)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) AsFloat64() float64 {
	return *(*float64)(unsafe.Pointer(&v.ptr.anon0[0]))
}

func (v *Value) SetNode(node Node) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_NODE

	*(*C.nbf_node_t)(unsafe.Pointer(&v.ptr.anon0[0])) = *node.node()
}

func (v *Value) SetList(list List) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_LIST

	*(*C.nbf_list_t)(unsafe.Pointer(&v.ptr.anon0[0])) = *list.ptr
}

func (v *Value) SetRaw(raw []byte) error {
	v.free_()

	v.ptr._type = C.NBF_TYPES_RAW

	r := (*C.nbf_raw_t)(
		unsafe.Pointer(&v.ptr.anon0[0]),
	)

	r.data = (*C.byte)(C.malloc(C.size_t(len(raw))))
	if r.data == nil {
		return errors.New("malloc failed")
	}
	v.tv_().__ownership = C.NBF_OWNERSHIP_HEAP
	r.size = C.uint32_t(len(raw))

	C.memcpy(
		unsafe.Pointer(r.data),
		unsafe.Pointer(&raw[0]),
		C.size_t(len(raw)),
	)

	return nil
}

func toCStr(s string) (*C.char, error) {
	size := len(s) + 1

	ptr := C.malloc(C.size_t(size))
	if ptr == nil {
		return nil, errors.New("failed to allocate C string")
	}

	copy((*[1 << 30]byte)(ptr)[:len(s)], s)

	(*(*byte)(unsafe.Add(ptr, len(s)))) = 0

	return (*C.char)(ptr), nil
}

func (v *Value) SetString(str string) error {
	v.free_()
	v.ptr._type = C.NBF_TYPES_STRING

	v.tv_().__ownership = C.NBF_OWNERSHIP_HEAP
	cstr, err := toCStr(str)

	if err != nil {
		return err
	}

	*(*unsafe.Pointer)(unsafe.Pointer(&v.ptr.anon0[0])) = unsafe.Pointer(cstr)

	return nil
}

func (v *Value) SetInt8(x int8) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_INT8

	*(*int8)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetInt16(x int16) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_INT16

	*(*int16)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetInt32(x int32) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_INT32

	*(*int32)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetInt64(x int64) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_INT64

	*(*int64)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetUint8(x uint8) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_UINT8

	*(*uint8)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetUint16(x uint16) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_UINT16

	*(*uint16)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetUint32(x uint32) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_UINT32

	*(*uint32)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetUint64(x uint64) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_UINT64

	*(*uint64)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetFloat32(x float32) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_FLOAT32

	*(*float32)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetFloat64(x float64) {
	v.free_()
	v.ptr._type = C.NBF_TYPES_FLOAT64

	*(*float64)(unsafe.Pointer(&v.ptr.anon0[0])) = x
}

func (v *Value) SetEmpty() {
	v.free_()
	v.ptr._type = C.NBF_TYPES_EMPTY

	*(*unsafe.Pointer)(unsafe.Pointer(&v.ptr.anon0[0])) = nil
}

func (v *Value) free_() {
	C.nbf_value_free(&v.ptr)
}
