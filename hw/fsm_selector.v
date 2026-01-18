`timescale 1ps/1ps
// ============================================================================
// mxfs_selector: Moore Machine Based Bus Ownership Selector
//
// Moore Machine Characteristics:
//   - Output Q depends only on the current state, not directly responding to input signal changes
//   - Output glitches during state transitions are suppressed, ensuring deterministic ownership switching
//
// State Table:
//   State | out_ctrl1_fast | out_ctrl1_slow | Q | Bus Ownership
//   ------|----------------|----------------|---|------------------
//   S0    | L              | L              | H | Mother (fast domain)
//   S1    | H              | L              | H | Mother (fast domain)
//   S2    | L              | H              | L | Child (slow domain)
//   S3    | H              | H              | H | Mother (fast domain)
//
// ============================================================================
module mxfs_selector (
    input  wire clk,
    input  wire rst_n,
    input  wire out_ctrl1_fast,
    input  wire out_ctrl1_slow,
    output reg  q
);

    // State definitions
    localparam [1:0] S0 = 2'b00;  // (L, L) -> Q = H
    localparam [1:0] S1 = 2'b01;  // (H, L) -> Q = H
    localparam [1:0] S2 = 2'b10;  // (L, H) -> Q = L
    localparam [1:0] S3 = 2'b11;  // (H, H) -> Q = H

    reg [1:0] state;
    reg [1:0] next_state;

    // Input synchronization registers (metastability mitigation)
    reg out_ctrl1_fast_sync1, out_ctrl1_fast_sync2;
    reg out_ctrl1_slow_sync1, out_ctrl1_slow_sync2;

    // Input synchronization (2-stage flip-flop)
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            out_ctrl1_fast_sync1 <= 1'b0;
            out_ctrl1_fast_sync2 <= 1'b0;
            out_ctrl1_slow_sync1 <= 1'b0;
            out_ctrl1_slow_sync2 <= 1'b0;
        end else begin
            out_ctrl1_fast_sync1 <= out_ctrl1_fast;
            out_ctrl1_fast_sync2 <= out_ctrl1_fast_sync1;
            out_ctrl1_slow_sync1 <= out_ctrl1_slow;
            out_ctrl1_slow_sync2 <= out_ctrl1_slow_sync1;
        end
    end

    // Next state logic (combinational logic)
    // Determine next state based on synchronized inputs
    always @(*) begin
        case ({out_ctrl1_fast_sync2, out_ctrl1_slow_sync2})
            2'b00: next_state = S0;
            2'b01: next_state = S2;
            2'b10: next_state = S1;
            2'b11: next_state = S3;
            default: next_state = S0;
        endcase
    end

    // State register (sequential logic)
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= S0;
        end else begin
            state <= next_state;
        end
    end

    // Output logic (Moore machine: depends only on state)
    // Registered output guarantees glitch-free operation
    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            q <= 1'b1;  // On reset, Mother (fast domain) owns the bus
        end else begin
            case (state)
                S0: q <= 1'b1;  // Mother (fast domain)
                S1: q <= 1'b1;  // Mother (fast domain)
                S2: q <= 1'b0;  // Child (slow domain)
                S3: q <= 1'b1;  // Mother (fast domain)
                default: q <= 1'b1;
            endcase
        end
    end

endmodule
