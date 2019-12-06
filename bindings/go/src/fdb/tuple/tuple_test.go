package tuple

import (
	"bytes"
	"encoding/gob"
	"flag"
	"fmt"
	"math/rand"
	"os"
	"testing"
)

var update = flag.Bool("update", false, "update .golden files")

func loadGolden(t *testing.T) (golden map[string][]byte) {
	f, err := os.Open("testdata/tuples.golden")
	if err != nil {
		t.Fatalf("failed to open golden file: %s", err)
	}
	defer f.Close()

	err = gob.NewDecoder(f).Decode(&golden)
	if err != nil {
		t.Fatalf("failed to decode golden file: %s", err)
	}
	return
}

func writeGolden(t *testing.T, golden map[string][]byte) {
	f, err := os.Create("testdata/tuples.golden")
	if err != nil {
		t.Fatalf("failed to open golden file: %s", err)
	}
	defer f.Close()

	err = gob.NewEncoder(f).Encode(golden)
	if err != nil {
		t.Fatalf("failed to encode golden file: %s", err)
	}
}

var testUUID = UUID{
	0x11, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x11, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
}

func genBytes() interface{}     { return []byte("namespace") }
func genBytesNil() interface{}  { return []byte{0xFF, 0x00, 0xFF} }
func genString() interface{}    { return "namespace" }
func genStringNil() interface{} { return "nam\x00es\xFFpace" }
func genInt() interface{}       { return rand.Int63() }
func genFloat() interface{}     { return float32(rand.NormFloat64()) }
func genDouble() interface{}    { return rand.NormFloat64() }

func mktuple(gen func() interface{}, count int) Tuple {
	tt := make(Tuple, count)
	for i := 0; i < count; i++ {
		tt[i] = gen()
	}
	return tt
}

var testCases = []struct {
	name  string
	tuple Tuple
}{
	{"Simple", Tuple{testUUID, "foobarbaz", 1234, nil}},
	{"Namespaces", Tuple{testUUID, "github", "com", "apple", "foundationdb", "tree"}},
	{"ManyStrings", mktuple(genString, 8)},
	{"ManyStringsNil", mktuple(genStringNil, 8)},
	{"ManyBytes", mktuple(genBytes, 20)},
	{"ManyBytesNil", mktuple(genBytesNil, 20)},
	{"LargeBytes", Tuple{testUUID, bytes.Repeat([]byte("abcd"), 20)}},
	{"LargeBytesNil", Tuple{testUUID, bytes.Repeat([]byte{0xFF, 0x0, 0xFF}, 20)}},
	{"Integers", mktuple(genInt, 20)},
	{"Floats", mktuple(genFloat, 20)},
	{"Doubles", mktuple(genDouble, 20)},
	{"UUIDs", Tuple{testUUID, true, testUUID, false, testUUID, true, testUUID, false, testUUID, true}},
	{"NilCases", Tuple{"\x00", "\x00\xFF", "\x00\x00\x00", "\xFF\x00", ""}},
	{"Nested", Tuple{testUUID, mktuple(genInt, 4), nil, mktuple(genBytes, 4), nil, mktuple(genDouble, 4), nil}},
}

func TestTuplePacking(t *testing.T) {
	var golden map[string][]byte

	if *update {
		golden = make(map[string][]byte)
	} else {
		golden = loadGolden(t)
	}

	for _, tt := range testCases {
		t.Run(tt.name, func(t *testing.T) {
			result := tt.tuple.Pack()

			if *update {
				golden[tt.name] = result
				return
			}

			if !bytes.Equal(result, golden[tt.name]) {
				t.Errorf("packing mismatch: expected %v, got %v", golden[tt.name], result)
			}
		})
	}

	if *update {
		writeGolden(t, golden)
	}
}

func BenchmarkTuplePacking(b *testing.B) {
	for _, bm := range testCases {
		b.Run(bm.name, func(b *testing.B) {
			tuple := bm.tuple
			for i := 0; i < b.N; i++ {
				_ = tuple.Pack()
			}
		})
	}
}

func TestTupleString(t *testing.T) {
	printed := fmt.Sprint(Tuple{[]byte("hello"), "world", 42, 0x99})
	expected := "\\x01hello\\x00\\x02world\\x00\\x15*\\x15\\x99"

	if printed != expected {
		t.Fatalf("printed tuple result differs, expected %v, got %v", expected, printed)
	}
}
