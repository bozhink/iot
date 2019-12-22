package main

import "testing"

func Test_ps(t *testing.T) {
	type args struct {
		t float64
		h float64
	}
	tests := []struct {
		name string
		args args
		want float64
	}{
		{
			name: "50%",
			args: args{t: 20, h: 50},
			want: 760.0,
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			if got := ps(tt.args.t, tt.args.h); got != tt.want {
				t.Errorf("ps() = %v, want %v", got, tt.want)
			}
		})
	}
}
