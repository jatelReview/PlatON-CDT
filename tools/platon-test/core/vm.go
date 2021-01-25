package core

import (
	"errors"
	"fmt"
	"math/big"
	"reflect"
	"time"

	"github.com/PlatONnetwork/wagon/exec"
	"github.com/PlatONnetwork/wagon/wasm"
)

var (
	ErrWASMRlpItemCountTooLarge = errors.New("WASM: itemCount too large for RLP")
	importer                    = func(name string) (*wasm.Module, error) {
		switch name {
		case "env":
			return NewHostModule(), nil
		}
		return nil, fmt.Errorf("module %q unknown", name)
	}
)

type VMContext struct {
	Input   []byte
	CallOut []byte
	Output  []byte
	Gas     *uint64
	OpCode  *uint64
	Db      DB
}

func addFuncExport(m *wasm.Module, sig wasm.FunctionSig, function wasm.Function, export wasm.ExportEntry) {
	typesLen := len(m.Types.Entries)
	m.Types.Entries = append(m.Types.Entries, sig)
	function.Sig = &m.Types.Entries[typesLen]
	funcLen := len(m.FunctionIndexSpace)
	m.FunctionIndexSpace = append(m.FunctionIndexSpace, function)
	export.Index = uint32(funcLen)
	m.Export.Entries[export.FieldStr] = export
}

func Debug(proc *exec.Process, dst uint32, len uint32) {
	buf := make([]byte, len)
	proc.ReadAt(buf, int64(dst))
	fmt.Printf("%s", string(buf))
}

func Panic(proc *exec.Process) {
	panic("test case panic")
}

func Revert(proc *exec.Process) {
	proc.Terminate()
}

func SetState(proc *exec.Process, key uint32, keyLen uint32, val uint32, valLen uint32) {
	db := proc.HostCtx().(*VMContext).Db
	keyBuf := make([]byte, keyLen)
	proc.ReadAt(keyBuf, int64(key))
	if valLen == 0 {
		db.Delete(string(keyBuf))
		return
	}
	valBuf := make([]byte, valLen)
	proc.ReadAt(valBuf, int64(val))
	db.Set(string(keyBuf), valBuf)
}

func GetStateLength(proc *exec.Process, key uint32, keyLen uint32) uint32 {
	db := proc.HostCtx().(*VMContext).Db
	keyBuf := make([]byte, keyLen)
	proc.ReadAt(keyBuf, int64(key))
	return uint32(len(db.Get(string(keyBuf))))
}

func GetState(proc *exec.Process, key uint32, keyLen uint32, val uint32, valLen uint32) int32 {
	db := proc.HostCtx().(*VMContext).Db
	keyBuf := make([]byte, keyLen)
	proc.ReadAt(keyBuf, int64(key))
	valBuf := db.Get(string(keyBuf))
	if uint32(len(valBuf)) > valLen {
		return -1
	}
	proc.WriteAt(valBuf, int64(val))
	return int32(len(valBuf))
}

func ReturnContract(proc *exec.Process, dst uint32, len uint32) {
	buf := make([]byte, len)
	proc.ReadAt(buf, int64(dst))
	fmt.Printf("platon_return:")
	fmt.Println(buf)
}

func DebugGas(proc *exec.Process, line uint32, dst uint32, len uint32) {
	buf := make([]byte, len)
	proc.ReadAt(buf, int64(dst))
	ctx := proc.HostCtx().(*VMContext)

	fmt.Println("debug gas:", "line:", line, "func:", string(buf), "gas:", *ctx.Gas, "opcode:", *ctx.OpCode)
}

func GetInputLength(proc *exec.Process) uint32 {
	ctx := proc.HostCtx().(*VMContext)
	return uint32(len(ctx.Input))
}

func GetInput(proc *exec.Process, dst uint32) {
	ctx := proc.HostCtx().(*VMContext)
	_, err := proc.WriteAt(ctx.Input, int64(dst))
	if err != nil {
		panic(err)
	}
}

// rlp encode
const (
	rlpMaxLengthBytes  = 8
	rlpDataImmLenStart = 0x80
	rlpListStart       = 0xc0
	rlpDataImmLenCount = rlpListStart - rlpDataImmLenStart - rlpMaxLengthBytes
	rlpDataIndLenZero  = rlpDataImmLenStart + rlpDataImmLenCount - 1
	rlpListImmLenCount = 256 - rlpListStart - rlpMaxLengthBytes
	rlpListIndLenZero  = rlpListStart + rlpListImmLenCount - 1
)

func bigEndian(num uint64) []byte {
	var (
		temp   []byte
		result []byte
	)

	for {
		if 0 == num {
			break
		}
		temp = append(temp, byte(num))
		num = num >> 8
	}

	for i := len(temp); i != 0; i = i - 1 {
		result = append(result, temp[i-1])
	}

	return result
}

// size_t rlp_u128_size(uint64_t heigh, uint64_t low);
func RlpU128Size(proc *exec.Process, heigh uint64, low uint64) uint32 {
	var size uint32 = 0
	if (0 == heigh && 0 == low) || (0 == heigh && low < rlpDataImmLenStart) {
		size = 1
	} else {
		dataLen := len(bigEndian(heigh)) + len(bigEndian(low))
		size = uint32(dataLen) + 1
	}

	return size
}

// void platon_rlp_u128(uint64_t heigh, uint64_t low, void * dest);
func RlpU128(proc *exec.Process, heigh uint64, low uint64, dest uint32) {
	// rlp result
	var data []byte
	if 0 == heigh && 0 == low {
		data = append(data, rlpDataImmLenStart)
	} else if 0 == heigh && low < rlpDataImmLenStart {
		data = append(data, byte(low))
	} else {
		temp := bigEndian(heigh)
		temp = append(temp, bigEndian(low)...)
		data = append(data, byte(rlpDataImmLenStart+len(temp)))
		data = append(data, temp...)
	}

	// write data
	_, err := proc.WriteAt(data, int64(dest))
	if nil != err {
		panic(err)
	}
}

// size_t rlp_bytes_size(const void *data, size_t len);
func RlpBytesSize(proc *exec.Process, src uint32, length uint32) uint32 {
	// read data
	data := make([]byte, 1)
	_, err := proc.ReadAt(data, int64(src))
	if nil != err {
		panic(err)
	}

	if 1 == length && data[0] < rlpDataImmLenStart {
		return 1
	}

	if length < rlpDataImmLenCount {
		return length + 1
	}

	return uint32(len(bigEndian(uint64(length)))) + length + 1
}

// void platon_rlp_bytes(const void *data, size_t len, void * dest);
func RlpBytes(proc *exec.Process, src uint32, length uint32, dest uint32) {
	// read data
	data := make([]byte, length)
	_, err := proc.ReadAt(data, int64(src))
	if nil != err {
		panic(err)
	}

	// get prefixData
	var prefixData []byte
	if 1 == length && data[0] < rlpDataImmLenStart {
		prefixData = []byte{}
	} else if length < rlpDataImmLenCount {
		prefixData = append(prefixData, byte(rlpDataImmLenStart+length))
	} else {
		lengthBytes := bigEndian(uint64(length))
		if len(lengthBytes)+rlpDataIndLenZero > 0xff {
			panic(ErrWASMRlpItemCountTooLarge)
		}
		prefixData = append(prefixData, byte(len(lengthBytes)+rlpDataIndLenZero))
		prefixData = append(prefixData, lengthBytes...)
	}

	// write prefix
	_, err = proc.WriteAt(prefixData, int64(dest))
	if nil != err {
		panic(err)
	}

	// write data
	_, err = proc.WriteAt(data, int64(dest)+int64(len(prefixData)))
	if nil != err {
		panic(err)
	}
}

// size_t rlp_list_size(const void *data, size_t len);
func RlpListSize(proc *exec.Process, length uint32) uint32 {
	if length < rlpListImmLenCount {
		return length + 1
	}

	return uint32(len(bigEndian(uint64(length)))) + length + 1
}

// void platon_rlp_list(const void *data, size_t len, void * dest);
func RlpList(proc *exec.Process, src uint32, length uint32, dest uint32) {
	// read data
	data := make([]byte, length)
	_, err := proc.ReadAt(data, int64(src))
	if nil != err {
		panic(err)
	}

	// get result
	var prefixData []byte
	if length < rlpListImmLenCount {
		prefixData = append(prefixData, byte(rlpListStart+length))
	} else {
		lengthBytes := bigEndian(uint64(length))
		if len(lengthBytes)+rlpDataIndLenZero > 0xff {
			panic(ErrWASMRlpItemCountTooLarge)
		}
		prefixData = append(prefixData, byte(len(lengthBytes)+rlpListIndLenZero))
		prefixData = append(prefixData, lengthBytes...)
	}

	// write prefix
	_, err = proc.WriteAt(prefixData, int64(dest))
	if nil != err {
		panic(err)
	}

	// write data
	_, err = proc.WriteAt(data, int64(dest)+int64(len(prefixData)))
	if nil != err {
		panic(err)
	}
}

// int64_t platon_nano_time()
func PlatonNanoTime(proc *exec.Process) int64 {
	return time.Now().UnixNano()
}

// uint8_t bigint_binary_operators(uint8_t *left, bool left_negative, uint8_t *right, bool right_negative, uint8_t *result, size_t arr_size, uint32_t operators);
// func $bigint_binary_operators(param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (param $5 i32) (param $6 i32) (result  i32)
type BinaryOperatorType uint32

const (
	BIGINTADD BinaryOperatorType = 0x01
	BIGINTSUB BinaryOperatorType = 0x02
	BIGINTMUL BinaryOperatorType = 0x04
	BIGINTDIV BinaryOperatorType = 0x08
	BIGINTMOD BinaryOperatorType = 0x10
	BIGINTAND BinaryOperatorType = 0x20
	BIGINTOR  BinaryOperatorType = 0x40
	BIGINTXOR BinaryOperatorType = 0x80
)

func BigintBinaryOperators(proc *exec.Process, left uint32, leftNegative uint32, right uint32, rightNegative uint32,
	result uint32, arrSize uint32, operators uint32) uint32 {
	// read left data
	dataBuf := make([]byte, arrSize)
	_, leftErr := proc.ReadAt(dataBuf, int64(left))
	if nil != leftErr {
		panic(leftErr)
	}

	// fmt.Println("BigintBinaryOperators left: ", dataBuf)

	leftOperand := new(big.Int)
	leftOperand.SetBytes(dataBuf)

	if leftNegative > 0 {
		bigValueTemp := new(big.Int)
		bigValueTemp.Neg(leftOperand)
		leftOperand = bigValueTemp
	}

	// read right data
	_, rightErr := proc.ReadAt(dataBuf, int64(right))
	if nil != rightErr {
		panic(rightErr)
	}

	// fmt.Println("BigintBinaryOperators right: ", dataBuf)

	rightOperand := new(big.Int)
	rightOperand.SetBytes(dataBuf)

	if rightNegative > 0 {
		bigValueTemp := new(big.Int)
		bigValueTemp.Neg(rightOperand)
		rightOperand = bigValueTemp
	}

	// binary operation
	operationResult := new(big.Int)
	switch BinaryOperatorType(operators) {
	case BIGINTADD:
		operationResult.Add(leftOperand, rightOperand)
	case BIGINTSUB:
		operationResult.Sub(leftOperand, rightOperand)
	case BIGINTMUL:
		operationResult.Mul(leftOperand, rightOperand)
	case BIGINTDIV:
		operationResult.Div(leftOperand, rightOperand)
	case BIGINTMOD:
		operationResult.Mod(leftOperand, rightOperand)
	case BIGINTAND:
		operationResult.And(leftOperand, rightOperand)
	case BIGINTOR:
		operationResult.Or(leftOperand, rightOperand)
	case BIGINTXOR:
		operationResult.Xor(leftOperand, rightOperand)
	}

	// fmt.Println(operators)

	// write result
	bytesResult := operationResult.Bytes()

	// fmt.Println("BigintBinaryOperators result: ", bytesResult, operationResult)

	// Clear to zero
	zeroResult := make([]byte, arrSize)
	_, clearErr := proc.WriteAt(zeroResult, int64(result))
	if nil != clearErr {
		panic(clearErr)
	}

	//Set to actual value
	lenResult := len(bytesResult)
	if lenResult > int(arrSize) {
		lenResult = int(arrSize)
	}
	result = result + arrSize - uint32(lenResult)

	_, setErr := proc.WriteAt(bytesResult, int64(result))
	if nil != setErr {
		panic(setErr)
	}

	var returnResult uint32 = 0

	if -1 == operationResult.Sign() {
		returnResult = 1
	}

	if len(bytesResult) > int(arrSize) {
		returnResult |= 0x02
	}

	return returnResult
}

// int bigint_cmp(uint8_t *left, uint8_t left_negative, uint8_t *right, uint8_t right_negative);
// func $bigint_cmp(param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (result  i32)
func BigintCmp(proc *exec.Process, left uint32, leftNegative uint32, right uint32, rightNegative uint32, arrSize uint32) int32 {
	// read left data
	dataBuf := make([]byte, arrSize)
	_, leftErr := proc.ReadAt(dataBuf, int64(left))
	if nil != leftErr {
		panic(leftErr)
	}

	leftOperand := new(big.Int)
	leftOperand.SetBytes(dataBuf)

	if leftNegative > 0 {
		bigValueTemp := new(big.Int)
		bigValueTemp.Neg(leftOperand)
		leftOperand = bigValueTemp
	}

	// read right data
	_, rightErr := proc.ReadAt(dataBuf, int64(right))
	if nil != rightErr {
		panic(rightErr)
	}

	rightOperand := new(big.Int)
	rightOperand.SetBytes(dataBuf)

	if rightNegative > 0 {
		bigValueTemp := new(big.Int)
		bigValueTemp.Neg(rightOperand)
		rightOperand = bigValueTemp
	}

	// compare
	result := int32(leftOperand.Cmp(rightOperand))
	return result
}

// uint8_t bigint_sh(uint8_t *origin, uint8_t origin_negative, uint32_t n, uint8_t *result, size_t arr_size, uint32_t direction);
// func $bigint_sh(param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (param $5 i32) (result i32)
type ShiftDirectionType uint32

const (
	LEFT  ShiftDirectionType = 0x01
	RIGHT ShiftDirectionType = 0x02
)

func BigintSh(proc *exec.Process, origin uint32, originNegative uint32, shiftNum uint32, result uint32, arrSize uint32, direction uint32) uint32 {
	// read origin data
	dataBuf := make([]byte, arrSize)
	_, originErr := proc.ReadAt(dataBuf, int64(origin))
	if nil != originErr {
		panic(originErr)
	}

	originOperand := new(big.Int)
	originOperand.SetBytes(dataBuf)

	if originNegative > 0 {
		bigValueTemp := new(big.Int)
		bigValueTemp.Neg(originOperand)
		originOperand = bigValueTemp
	}

	// shift
	operationResult := new(big.Int)
	switch ShiftDirectionType(direction) {
	case LEFT:
		operationResult.Lsh(originOperand, uint(shiftNum))
	case RIGHT:
		operationResult.Rsh(originOperand, uint(shiftNum))
	}

	// write result
	bytesResult := operationResult.Bytes()

	// Clear to zero
	zeroResult := make([]byte, arrSize)
	_, clearErr := proc.WriteAt(zeroResult, int64(result))
	if nil != clearErr {
		panic(clearErr)
	}

	//Set to actual value
	lenResult := len(bytesResult)
	if lenResult > int(arrSize) {
		lenResult = int(arrSize)
	}
	result = result + arrSize - uint32(lenResult)

	_, setErr := proc.WriteAt(bytesResult, int64(result))
	if nil != setErr {
		panic(setErr)
	}

	var returnResult uint32 = 0

	if -1 == operationResult.Sign() {
		returnResult = 1
	}

	if len(bytesResult) > int(arrSize) {
		returnResult |= 0x02
	}

	return returnResult
}

func NewHostModule() *wasm.Module {
	m := wasm.NewModule()
	m.Export.Entries = make(map[string]wasm.ExportEntry)

	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes: []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(Debug),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_debug",
			Kind:     wasm.ExternalFunction,
		},
	)

	addFuncExport(m,
		wasm.FunctionSig{},
		wasm.Function{
			Host: reflect.ValueOf(Panic),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_panic",
			Kind:     wasm.ExternalFunction,
		},
	)

	addFuncExport(m,
		wasm.FunctionSig{},
		wasm.Function{
			Host: reflect.ValueOf(Revert),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_revert",
			Kind:     wasm.ExternalFunction,
		},
	)

	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes:  []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32},
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(GetState),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_get_state",
			Kind:     wasm.ExternalFunction,
		},
	)

	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes:  []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32},
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{

			Host: reflect.ValueOf(GetStateLength),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_get_state_length",
			Kind:     wasm.ExternalFunction,
		},
	)

	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes: []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(SetState),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_set_state",
			Kind:     wasm.ExternalFunction,
		},
	)

	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes: []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(ReturnContract),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_return",
			Kind:     wasm.ExternalFunction,
		},
	)

	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes: []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(DebugGas),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_debug_gas",
			Kind:     wasm.ExternalFunction,
		},
	)
	addFuncExport(m,
		wasm.FunctionSig{
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(GetInputLength),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_get_input_length",
			Kind:     wasm.ExternalFunction,
		},
	)

	// void platon_get_input(const uint8_t *value)
	// func $platon_get_input (param $0 i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(GetInput),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_get_input",
			Kind:     wasm.ExternalFunction,
		},
	)

	// size_t rlp_u128_size(uint64_t heigh, uint64_t low);
	// func rlp_u128_size (param $0 i64) (param $1 i64) (result i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes:  []wasm.ValueType{wasm.ValueTypeI64, wasm.ValueTypeI64},
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(RlpU128Size),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "rlp_u128_size",
			Kind:     wasm.ExternalFunction,
		},
	)

	// void platon_rlp_u128(uint64_t heigh, uint64_t low, void * dest);
	// func platon_rlp_u128 (param $0 i64) (param $1 i64) (param $2 i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes: []wasm.ValueType{wasm.ValueTypeI64, wasm.ValueTypeI64, wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(RlpU128),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_rlp_u128",
			Kind:     wasm.ExternalFunction,
		},
	)

	// size_t rlp_bytes_size(const void *data, size_t len);
	// func rlp_bytes_size (param $0 i32) (param $1 i32) (result i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes:  []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32},
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(RlpBytesSize),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "rlp_bytes_size",
			Kind:     wasm.ExternalFunction,
		},
	)

	// void platon_rlp_bytes(const void *data, size_t len, void * dest);
	// func platon_rlp_bytes (param $0 i32) (param $1 i32) (param $2 i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes: []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(RlpBytes),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_rlp_bytes",
			Kind:     wasm.ExternalFunction,
		},
	)

	// size_t rlp_list_size(size_t len);
	// func rlp_list_size (param $0 i32) (result i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes:  []wasm.ValueType{wasm.ValueTypeI32},
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(RlpListSize),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "rlp_list_size",
			Kind:     wasm.ExternalFunction,
		},
	)

	// void platon_rlp_list(const void *data, size_t len, void * dest);
	// func platon_rlp_list (param $0 i32) (param $1 i32) (param $2 i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes: []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(RlpList),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_rlp_list",
			Kind:     wasm.ExternalFunction,
		},
	)

	addFuncExport(m,
		wasm.FunctionSig{
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI64},
		},
		wasm.Function{
			Host: reflect.ValueOf(PlatonNanoTime),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "platon_nano_time",
			Kind:     wasm.ExternalFunction,
		},
	)

	// uint8_t bigint_binary_operators(uint8_t *left, bool left_negative, uint8_t *right, bool right_negative, uint8_t *result, size_t arr_size, uint32_t operators);
	// func $bigint_binary_operators(param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (param $5 i32) (param $6 i32) (result  i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes:  []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32},
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(BigintBinaryOperators),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "bigint_binary_operators",
			Kind:     wasm.ExternalFunction,
		},
	)

	// int bigint_cmp(uint8_t *left, uint8_t left_negative, uint8_t *right, uint8_t right_negative, size_t arr_size);
	// func $bigint_cmp(param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (result i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes:  []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32},
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(BigintCmp),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "bigint_cmp",
			Kind:     wasm.ExternalFunction,
		},
	)

	// uint8_t bigint_sh(uint8_t *origin, uint8_t origin_negative, uint32_t n, uint8_t *result, size_t arr_size, uint32_t direction);
	// func $bigint_sh(param $0 i32) (param $1 i32) (param $2 i32) (param $3 i32) (param $4 i32) (param $5 i32) (result i32)
	addFuncExport(m,
		wasm.FunctionSig{
			ParamTypes:  []wasm.ValueType{wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32, wasm.ValueTypeI32},
			ReturnTypes: []wasm.ValueType{wasm.ValueTypeI32},
		},
		wasm.Function{
			Host: reflect.ValueOf(BigintSh),
			Body: &wasm.FunctionBody{},
		},
		wasm.ExportEntry{
			FieldStr: "bigint_sh",
			Kind:     wasm.ExternalFunction,
		},
	)

	return m
}
