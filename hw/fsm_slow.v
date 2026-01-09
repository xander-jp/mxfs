`timescale 1ps/1ps
module mxfs_slow (
    input wire in_ctrl,
    input wire dp_clk,
    input wire upl_arst_n,
    input wire [31:0] debug_status_ingress,  // ソフトウェアからの入力 (out_ctrl0 含む)
    output reg out_ctrl0,
    output reg out_ctrl1,
    output reg [31:0] debug_status_egress    // モジュールからの出力
  );
  reg [3:0]          __internal_status;
  reg [3:0]          __internal_status_next;
  reg                __in_ctrl;
  reg [15:0]         __counter;
  reg                __out_ctrl0_sw;         // ソフトウェア制御の out_ctrl0

  
  // Status
  parameter MXFS_STATUS_INIT     = 4'd1;
  parameter MXFS_STATUS_READY_0  = 4'd2;
  parameter MXFS_STATUS_READY_1  = 4'd3;
  parameter MXFS_STATUS_VALID    = 4'd4;
  parameter ON = 1'b1;
  parameter OFF= 1'b0;
  
  // clock, asynchronized reset
  always @(posedge dp_clk or negedge upl_arst_n) begin
    if (!upl_arst_n) begin
      __internal_status      <= MXFS_STATUS_INIT;
      __internal_status_next <= MXFS_STATUS_INIT;
      __counter              <= 16'd0;
    end else begin
      __internal_status <= __internal_status_next;

      case (__internal_status_next)
        MXFS_STATUS_INIT: begin
          if (__counter >= 16'd200) begin
            __internal_status_next <= MXFS_STATUS_READY_0;
            __counter <= 0;
          end else begin
            __counter <= __counter + 1;
          end
        end

        MXFS_STATUS_READY_0: begin
          if (__counter >= 16'd200) begin
            __internal_status_next <= MXFS_STATUS_READY_1;
            __counter <= 0;
          end else begin
            __counter <= __counter + 1;
          end
        end

        MXFS_STATUS_READY_1: begin
          if (__counter >= 16'd200) begin
            __internal_status_next <= MXFS_STATUS_VALID;
            __counter <= 0;
          end else begin
            __counter <= __counter + 1;
          end
        end

        MXFS_STATUS_VALID: begin
          __counter <= 0;
        end
      endcase
    end
  end

  // in_ctrl を 1 clock 遅延で __in_ctrl に取り込む
  always @ (posedge dp_clk or negedge upl_arst_n) begin
    if (!upl_arst_n) begin
      __in_ctrl <= OFF;
    end else begin
      __in_ctrl <= in_ctrl;
    end
  end

  // debug_status_ingress から out_ctrl0 を取り込む (ソフトウェア制御)
  always @ (posedge dp_clk or negedge upl_arst_n) begin
    if (!upl_arst_n) begin
      __out_ctrl0_sw <= OFF;
    end else begin
      __out_ctrl0_sw <= debug_status_ingress[0];
    end
  end

  always @ (posedge dp_clk)
  begin
    // out_ctrl0 はソフトウェア制御
    out_ctrl0 <= __out_ctrl0_sw;

    // out_ctrl1 は FSM 状態と __in_ctrl で決まる
    if (__internal_status_next == MXFS_STATUS_INIT) begin
      out_ctrl1 <= OFF;
    end else if (__internal_status_next == MXFS_STATUS_READY_0) begin
      out_ctrl1 <= OFF;
    end else if (__internal_status_next == MXFS_STATUS_READY_1) begin
      out_ctrl1 <= ON;
    end else if (__internal_status_next == MXFS_STATUS_VALID) begin
      out_ctrl1 <= __in_ctrl;  // VALID時は __in_ctrl で制御
    end

    // debug_status_egress 出力
    debug_status_egress[31:28] <= __internal_status;
    debug_status_egress[27:24] <= __internal_status_next;
    debug_status_egress[15:0]  <= __counter;
  end

endmodule