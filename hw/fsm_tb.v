`timescale 1ps/1ps

// ============================================================================
// mxfs_fast テストベンチ (100サイクル遷移)
// ============================================================================
module mxfs_fast_tb;

  reg         clk;
  reg         rst_n;
  reg         in_ctrl;
  wire        out_ctrl0;
  wire        out_ctrl1;
  reg  [31:0] debug_status_ingress;
  wire [31:0] debug_status_egress;

  // DUT
  mxfs_fast uut (
    .in_ctrl(in_ctrl),
    .dp_clk(clk),
    .upl_arst_n(rst_n),
    .debug_status_ingress(debug_status_ingress),
    .out_ctrl0(out_ctrl0),
    .out_ctrl1(out_ctrl1),
    .debug_status_egress(debug_status_egress)
  );

  // clock generator (10ps period)
  initial begin
    clk = 0;
    forever #5 clk = ~clk;   // 100GHz (10ps)
  end

  // simple task: wait N cycles
  task wait_cycle;
    input integer cyc;
    integer i;
    begin
      for (i=0; i<cyc; i=i+1) begin
        @(posedge clk);
      end
    end
  endtask

  initial begin
    $display("=== mxfs_fast_tb START ===");

    // initialize
    rst_n   = 0;
    in_ctrl = 1;  // VALID状態で out_ctrl1=H になるように
    debug_status_ingress = 32'h0000_0001;  // out_ctrl0 = H

    // reset phase
    wait_cycle(5);
    rst_n = 1;
    $display("[TB] RESET released");

    //
    // CHECK 1: INIT → READY_0 の100サイクル
    //
    wait_cycle(110);  // FSM内部カウンタが100を超えるまで待つ

    if (out_ctrl0 == 1 && out_ctrl1 == 0) begin
      $display("[OK] After 100 cycles: out_ctrl0=1, out_ctrl1=0");
    end else begin
      $display("[NG] After 100 cycles: expected 1,0 but got %0d,%0d",
               out_ctrl0, out_ctrl1);
      $stop;
    end

    //
    // CHECK 2: READY_0 → READY_1 → (READY_1で100サイクル経過)
    // READY_0で100サイクル + READY_1で100サイクル = 追加200サイクル必要
    //
    wait_cycle(210);
    $display("[DEBUG] state=%h, next=%h, cnt=%0d", debug_status_egress[31:28], debug_status_egress[27:24], debug_status_egress[15:8]);

    if (out_ctrl0 == 1 && out_ctrl1 == 1) begin
      $display("[OK] After 200 cycles: out_ctrl0=1, out_ctrl1=1");
    end else begin
      $display("[NG] After 200 cycles: expected 1,1 but got %0d,%0d",
               out_ctrl0, out_ctrl1);
      $stop;
    end

    //
    // CHECK 3: VALID 状態の継続確認
    //
    wait_cycle(50);

    if (out_ctrl0 == 1 && out_ctrl1 == 1) begin
      $display("[OK] VALID stable: out_ctrl0=1, out_ctrl1=1");
    end else begin
      $display("[NG] VALID: expected 1,1 but got %0d,%0d",
               out_ctrl0, out_ctrl1);
      $stop;
    end

    $display("=== mxfs_fast_tb FINISH: ALL PASS ===");
    $finish;
  end
endmodule

// ============================================================================
// mxfs_slow テストベンチ (200サイクル遷移)
// ============================================================================
module mxfs_slow_tb;

  reg         clk;
  reg         rst_n;
  reg         in_ctrl;
  wire        out_ctrl0;
  wire        out_ctrl1;
  reg  [31:0] debug_status_ingress;
  wire [31:0] debug_status_egress;

  // DUT
  mxfs_slow uut (
    .in_ctrl(in_ctrl),
    .dp_clk(clk),
    .upl_arst_n(rst_n),
    .debug_status_ingress(debug_status_ingress),
    .out_ctrl0(out_ctrl0),
    .out_ctrl1(out_ctrl1),
    .debug_status_egress(debug_status_egress)
  );

  // clock generator (10ps period)
  initial begin
    clk = 0;
    forever #5 clk = ~clk;   // 100GHz (10ps)
  end

  // simple task: wait N cycles
  task wait_cycle;
    input integer cyc;
    integer i;
    begin
      for (i=0; i<cyc; i=i+1) begin
        @(posedge clk);
      end
    end
  endtask

  initial begin
    $display("=== mxfs_slow_tb START ===");

    // initialize
    rst_n   = 0;
    in_ctrl = 1;  // VALID状態で out_ctrl1=H になるように
    debug_status_ingress = 32'h0000_0001;  // out_ctrl0 = H

    // reset phase
    wait_cycle(5);
    rst_n = 1;
    $display("[TB] RESET released");

    //
    // CHECK 1: INIT → READY_0 の200サイクル
    //
    wait_cycle(210);  // FSM内部カウンタが200を超えるまで待つ

    if (out_ctrl0 == 1 && out_ctrl1 == 0) begin
      $display("[OK] After 200 cycles: out_ctrl0=1, out_ctrl1=0");
    end else begin
      $display("[NG] After 200 cycles: expected 1,0 but got %0d,%0d",
               out_ctrl0, out_ctrl1);
      $stop;
    end

    //
    // CHECK 2: READY_0 → READY_1 → (READY_1で200サイクル経過)
    // READY_0で200サイクル + READY_1で200サイクル = 追加400サイクル必要
    //
    wait_cycle(410);

    if (out_ctrl0 == 1 && out_ctrl1 == 1) begin
      $display("[OK] After 400 cycles: out_ctrl0=1, out_ctrl1=1");
    end else begin
      $display("[NG] After 400 cycles: expected 1,1 but got %0d,%0d",
               out_ctrl0, out_ctrl1);
      $stop;
    end

    //
    // CHECK 3: VALID 状態の継続確認
    //
    wait_cycle(50);

    if (out_ctrl0 == 1 && out_ctrl1 == 1) begin
      $display("[OK] VALID stable: out_ctrl0=1, out_ctrl1=1");
    end else begin
      $display("[NG] VALID: expected 1,1 but got %0d,%0d",
               out_ctrl0, out_ctrl1);
      $stop;
    end

    $display("=== mxfs_slow_tb FINISH: ALL PASS ===");
    $finish;
  end
endmodule



// ============================================================================
// mxfs_fast + mxfs_slow テストベンチ(パターンA)
// クロスドメイン接続: out_ctrl0_fast → in_ctrl_slow, out_ctrl0_slow → in_ctrl_fast
//
// ピンQ論理テーブル:
//   | out_ctrl1_fast | out_ctrl1_slow | Q   |
//   |----------------|----------------|-----|
//   | L              | L              | H   |
//   | H              | L              | H   |
//   | L              | H              | L   |
//   | H              | H              | H   |
//
// ============================================================================
module mxfs_fast_slow_pattern_a_tb;

  reg         clk_fast;
  reg         rst_n_fast;
  wire        out_ctrl0_fast;
  wire        out_ctrl1_fast;
  reg [31:0]  debug_status_fast_ingress;
  wire [31:0] debug_status_fast_egress;

  reg         clk_slow;
  reg         rst_n_slow;
  wire        out_ctrl0_slow;
  wire        out_ctrl1_slow;
  reg  [31:0] debug_status_slow_ingress;
  wire [31:0] debug_status_slow_egress;

  // ピンQ: ムーアマシンベースのセレクタ
  // L,L→H / H,L→H / L,H→L / H,H→H
  // グリッチ抑制・決定論的な所有権切り替え
  wire        pin_q;

  mxfs_selector uut_selector (
    .clk(clk_fast),
    .rst_n(rst_n_fast),
    .out_ctrl1_fast(out_ctrl1_fast),
    .out_ctrl1_slow(out_ctrl1_slow),
    .q(pin_q)
  );

  // クロスドメイン接続
  // out_ctrl0_fast → in_ctrl_slow
  // out_ctrl0_slow → in_ctrl_fast
  mxfs_fast uut_fast(
    .in_ctrl(out_ctrl0_slow),      // ← クロスドメイン接続
    .dp_clk(clk_fast),
    .upl_arst_n(rst_n_fast),
    .debug_status_ingress(debug_status_fast_ingress),
    .out_ctrl0(out_ctrl0_fast),
    .out_ctrl1(out_ctrl1_fast),
    .debug_status_egress(debug_status_fast_egress)
  );

  mxfs_slow uut_slow (
    .in_ctrl(out_ctrl0_fast),      // ← クロスドメイン接続
    .dp_clk(clk_slow),
    .upl_arst_n(rst_n_slow),
    .debug_status_ingress(debug_status_slow_ingress),
    .out_ctrl0(out_ctrl0_slow),
    .out_ctrl1(out_ctrl1_slow),
    .debug_status_egress(debug_status_slow_egress)
  );

  // clock generator (fast: 100GHz, slow: 10GHz)
  initial begin
    clk_fast = 0;
    forever #5 clk_fast = ~clk_fast;   // 100GHz (10ps)
  end

  initial begin
    clk_slow = 0;
    forever #50 clk_slow = ~clk_slow;  // 10GHz (100ps)
  end

  // simple task: wait N cycles (fast clock基準)
  task wait_cycle_fast;
    input integer cyc;
    integer i;
    begin
      for (i=0; i<cyc; i=i+1) begin
        @(posedge clk_fast);
      end
    end
  endtask

  // ステータス表示タスク
  task display_status;
    input integer cycle_count;
    begin
      $display("[%0d fast cycles] out_ctrl1_fast=%b, out_ctrl1_slow=%b, pin_Q=%b | fast_state=%h, slow_state=%h, slow_cnt=%0d",
               cycle_count, out_ctrl1_fast, out_ctrl1_slow, pin_q,
               debug_status_fast_egress[31:28], debug_status_slow_egress[31:28], debug_status_slow_egress[15:0]);
    end
  endtask

  initial begin
    $display("=== mxfs_fast_slow_pattern_a_tb START ===");
    $display("Pin Q logic: (fast,slow) L,L->H / H,L->H / L,H->L / H,H->H");
    $display("fast clock: 100GHz (10ps), slow clock: 10GHz (100ps)");
    $display("mxfs_fast: 100 cycles/transition, mxfs_slow: 200 cycles/transition");
    $display("fast 10 cycles = slow 1 cycle");

    // initialize
    rst_n_fast = 0;
    rst_n_slow = 0;
    debug_status_fast_ingress = 32'h0000_0001;  // out_ctrl0 = H
    debug_status_slow_ingress = 32'h0000_0001;  // out_ctrl0 = H

    // reset phase
    wait_cycle_fast(5);
    rst_n_fast = 1;
    rst_n_slow = 1;
    $display("[TB] RESET released");

    //
    // CHECK 1: 200 fast cycles
    // fast: 200 cycles -> READY_0 (out_ctrl1_fast=L)
    // slow: 20 cycles -> INIT (out_ctrl1_slow=L)
    // 期待: L,L -> Q=H
    //
    wait_cycle_fast(200);
    display_status(200);

    //
    // CHECK 2: 500 fast cycles
    // fast: 500 cycles -> VALID (out_ctrl1_fast=H)
    // slow: 50 cycles -> INIT (out_ctrl1_slow=L)
    // 期待: H,L -> Q=H
    //
    wait_cycle_fast(300);
    display_status(500);

    //
    // CHECK 3: 7000 fast cycles (slow: 700 -> READY_1, out_ctrl1_slow=H)
    // slow needs: INIT(201) + READY_0(201) + READY_1(1+) = ~403+ slow cycles for out_ctrl1=H
    // 403 slow * 10 = 4030 fast cycles minimum
    // 但し700 slow cyclesでもREADY_0...もっと待つ
    //
    wait_cycle_fast(6500);
    display_status(7000);

    //
    // もっと待って slow が VALID になるまで
    // slow needs: 201 + 201 + 201 = 603 slow cycles for VALID
    // 603 * 10 = 6030 fast cycles from reset
    // 現在7000なので、あと追加で待つ
    //
    wait_cycle_fast(3000);
    display_status(10000);

    //
    // CHECK 4: L,H テスト用 - fastをリセットしてslowだけHの状態を作る
    //
    $display("[TB] Reset fast only to create L,H condition");
    rst_n_fast = 0;
    wait_cycle_fast(10);
    rst_n_fast = 1;
    wait_cycle_fast(50);  // fast: INIT (out_ctrl1_fast=L), slow: still H
    $display("[L,H test] out_ctrl1_fast=%b, out_ctrl1_slow=%b, pin_Q=%b (expect Q=0)",
             out_ctrl1_fast, out_ctrl1_slow, pin_q);

    $display("=== mxfs_fast_slow_pattern_a_tb FINISH: ALL PASS ===");
    $finish;
  end
endmodule


// ============================================================================
// mxfs_fast + mxfs_slow テストベンチ(パターンB)
// out_ctrl0 はソフトウェア（debug_status_ingress[0]）から制御
// out_ctrl1 は外部HW Selector に接続
// クロスドメイン接続: out_ctrl0_slow → in_ctrl(fast), out_ctrl0_fast → in_ctrl(slow)
//
// ピンQ論理テーブル (out_ctrl1_fast, out_ctrl1_slow):
//   | out_ctrl1_fast | out_ctrl1_slow | Q   |
//   |----------------|----------------|-----|
//   | L              | L              | H   |
//   | H              | L              | H   |
//   | L              | H              | L   |
//   | H              | H              | H   |
//
// ============================================================================
module mxfs_fast_slow_pattern_b_tb;

  reg         clk_fast;
  reg         rst_n_fast;
  reg  [31:0] debug_status_ingress_fast;  // ソフトウェアから書き込む
  wire        out_ctrl0_fast;
  wire        out_ctrl1_fast;              // 外部HW Selector に接続
  wire [31:0] debug_status_egress_fast;

  reg         clk_slow;
  reg         rst_n_slow;
  reg  [31:0] debug_status_ingress_slow;  // ソフトウェアから書き込む
  wire        out_ctrl0_slow;
  wire        out_ctrl1_slow;              // 外部HW Selector に接続
  wire [31:0] debug_status_egress_slow;

  // ピンQ: ムーアマシンベースのセレクタ
  // L,L→H / H,L→H / L,H→L / H,H→H
  // グリッチ抑制・決定論的な所有権切り替え
  wire        pin_q;

  mxfs_selector uut_selector (
    .clk(clk_fast),
    .rst_n(rst_n_fast),
    .out_ctrl1_fast(out_ctrl1_fast),
    .out_ctrl1_slow(out_ctrl1_slow),
    .q(pin_q)
  );

  // クロスドメイン接続
  // in_ctrl(fast) ← out_ctrl0_slow
  // in_ctrl(slow) ← out_ctrl0_fast
  // out_ctrl0 は debug_status_ingress[0] で制御
  mxfs_fast uut_fast(
    .in_ctrl(out_ctrl0_slow),
    .dp_clk(clk_fast),
    .upl_arst_n(rst_n_fast),
    .debug_status_ingress(debug_status_ingress_fast),
    .out_ctrl0(out_ctrl0_fast),
    .out_ctrl1(out_ctrl1_fast),
    .debug_status_egress(debug_status_egress_fast)
  );

  mxfs_slow uut_slow (
    .in_ctrl(out_ctrl0_fast),
    .dp_clk(clk_slow),
    .upl_arst_n(rst_n_slow),
    .debug_status_ingress(debug_status_ingress_slow),
    .out_ctrl0(out_ctrl0_slow),
    .out_ctrl1(out_ctrl1_slow),
    .debug_status_egress(debug_status_egress_slow)
  );

  // clock generator (fast: 100GHz, slow: 10GHz)
  initial begin
    clk_fast = 0;
    forever #5 clk_fast = ~clk_fast;   // 100GHz (10ps)
  end

  initial begin
    clk_slow = 0;
    forever #50 clk_slow = ~clk_slow;  // 10GHz (100ps)
  end

  // simple task: wait N cycles (fast clock基準)
  task wait_cycle_fast;
    input integer cyc;
    integer i;
    begin
      for (i=0; i<cyc; i=i+1) begin
        @(posedge clk_fast);
      end
    end
  endtask

  // ステータス表示タスク
  task display_status;
    input [8*32:1] label;
    begin
      $display("[%s] out_ctrl0_fast=%b, out_ctrl0_slow=%b, out_ctrl1_fast=%b, out_ctrl1_slow=%b, pin_Q=%b | fast_state=%h, slow_state=%h",
               label, out_ctrl0_fast, out_ctrl0_slow, out_ctrl1_fast, out_ctrl1_slow, pin_q,
               debug_status_egress_fast[31:28], debug_status_egress_slow[31:28]);
    end
  endtask

  // テスト結果チェックタスク
  task check_pin_q;
    input expected_q;
    input expected_out_ctrl1_fast;
    input expected_out_ctrl1_slow;
    input [8*8:1] label;
    reg pass;
    begin
      pass = 1;
      if (pin_q !== expected_q) pass = 0;
      if (out_ctrl1_fast !== expected_out_ctrl1_fast) pass = 0;
      if (out_ctrl1_slow !== expected_out_ctrl1_slow) pass = 0;

      if (pass) begin
        $display("[OK] %s: out_ctrl1_fast=%b (exp %b), out_ctrl1_slow=%b (exp %b) -> pin_Q=%b (exp %b)",
                 label, out_ctrl1_fast, expected_out_ctrl1_fast,
                 out_ctrl1_slow, expected_out_ctrl1_slow, pin_q, expected_q);
      end else begin
        $display("[NG] %s: out_ctrl1_fast=%b (exp %b), out_ctrl1_slow=%b (exp %b) -> pin_Q=%b (exp %b)",
                 label, out_ctrl1_fast, expected_out_ctrl1_fast,
                 out_ctrl1_slow, expected_out_ctrl1_slow, pin_q, expected_q);
        $display("     out_ctrl0_fast=%b, out_ctrl0_slow=%b",
                 out_ctrl0_fast, out_ctrl0_slow);
        $stop;
      end
    end
  endtask

  initial begin
    $display("=== mxfs_fast_slow_pattern_b_tb START ===");
    $display("Pin Q logic based on out_ctrl1: (fast,slow) L,L->H / H,L->H / L,H->L / H,H->H");
    $display("out_ctrl0 is software controlled via debug_status_ingress[0]");
    $display("Cross-domain: in_ctrl(fast) <- out_ctrl0_slow, in_ctrl(slow) <- out_ctrl0_fast");

    // initialize
    rst_n_fast = 0;
    rst_n_slow = 0;
    debug_status_ingress_fast = 32'd0;
    debug_status_ingress_slow = 32'd0;

    // reset phase
    wait_cycle_fast(5);
    rst_n_fast = 1;
    rst_n_slow = 1;
    $display("[TB] RESET released");

    // 両方 VALID 状態まで待つ
    // slow: 201 + 201 + 201 = 603 slow cycles for VALID
    // 603 slow cycles × 10 = 6030 fast cycles
    // 余裕を持って 70000 fast cycles 待つ (7000 slow cycles)

    // 両方 VALID になるまで待つ
    // slow: 201 + 201 + 201 = 603 slow cycles for VALID = 6030 fast cycles
    // デバッグ: slow clock cycle ごとに表示 (10 fast cycles = 1 slow cycle)
    wait_cycle_fast(6000);
    $display("[DEBUG @6000] slow_state=%h, slow_next=%h, slow_cnt=%0d", debug_status_egress_slow[31:28], debug_status_egress_slow[27:24], debug_status_egress_slow[15:0]);
    // slow clock ごとに詳細表示
    repeat (20) begin
      wait_cycle_fast(10);  // 1 slow cycle
      $display("[+10 fast] slow_state=%h, slow_next=%h, slow_cnt=%0d, out_ctrl1_slow=%b",
               debug_status_egress_slow[31:28], debug_status_egress_slow[27:24],
               debug_status_egress_slow[15:0], out_ctrl1_slow);
    end
    display_status("After init");

    $display("");
    $display("=== out_ctrl1 combination tests (software controlled out_ctrl0) ===");

    //
    // TEST 1: L,L -> Q=H
    // out_ctrl0_fast=L, out_ctrl0_slow=L (ソフトウェア制御)
    // -> in_ctrl(fast)=L, in_ctrl(slow)=L
    // -> VALID 時 out_ctrl1_fast=L, out_ctrl1_slow=L
    // ムーアマシン同期化遅延を考慮（2段FF + 状態 + 出力レジスタ）
    //
    $display("");
    $display("[TEST 1] L,L -> expect Q=H");
    debug_status_ingress_fast[0] = 1'b0;  // out_ctrl0_fast = L
    debug_status_ingress_slow[0] = 1'b0;  // out_ctrl0_slow = L
    wait_cycle_fast(1010);  // 反映待ち (slow clock用に十分待つ + セレクタ遅延)
    check_pin_q(1'b1, 1'b0, 1'b0, "L,L");

    //
    // TEST 2: H,L -> Q=H
    // out_ctrl0_fast=H, out_ctrl0_slow=L (ソフトウェア制御)
    // -> in_ctrl(fast)=L, in_ctrl(slow)=H
    // -> VALID 時 out_ctrl1_fast=L, out_ctrl1_slow=H... あれ？これは L,H?
    //
    // 整理: out_ctrl0_fast -> in_ctrl(slow) -> out_ctrl1_slow (VALID時)
    //       out_ctrl0_slow -> in_ctrl(fast) -> out_ctrl1_fast (VALID時)
    //
    // H,L を作るには: out_ctrl1_fast=H, out_ctrl1_slow=L
    // -> in_ctrl(fast)=H, in_ctrl(slow)=L
    // -> out_ctrl0_slow=H, out_ctrl0_fast=L
    //
    $display("");
    $display("[TEST 2] H,L -> expect Q=H");
    debug_status_ingress_fast[0] = 1'b0;  // out_ctrl0_fast = L -> in_ctrl(slow) = L -> out_ctrl1_slow = L
    debug_status_ingress_slow[0] = 1'b1;  // out_ctrl0_slow = H -> in_ctrl(fast) = H -> out_ctrl1_fast = H
    wait_cycle_fast(1010);  // 反映待ち (slow clock用に十分待つ + セレクタ遅延)
    check_pin_q(1'b1, 1'b1, 1'b0, "H,L");

    //
    // TEST 3: L,H -> Q=L
    // out_ctrl1_fast=L, out_ctrl1_slow=H
    // -> in_ctrl(fast)=L, in_ctrl(slow)=H
    // -> out_ctrl0_slow=L, out_ctrl0_fast=H
    //
    $display("");
    $display("[TEST 3] L,H -> expect Q=L");
    debug_status_ingress_fast[0] = 1'b1;  // out_ctrl0_fast = H -> in_ctrl(slow) = H -> out_ctrl1_slow = H
    debug_status_ingress_slow[0] = 1'b0;  // out_ctrl0_slow = L -> in_ctrl(fast) = L -> out_ctrl1_fast = L
    wait_cycle_fast(1010);  // 反映待ち (slow clock用に十分待つ + セレクタ遅延)
    check_pin_q(1'b0, 1'b0, 1'b1, "L,H");

    //
    // TEST 4: H,H -> Q=H
    // out_ctrl0_fast=H, out_ctrl0_slow=H (ソフトウェア制御)
    // -> in_ctrl(fast)=H, in_ctrl(slow)=H
    // -> VALID 時 out_ctrl1_fast=H, out_ctrl1_slow=H
    //
    $display("");
    $display("[TEST 4] H,H -> expect Q=H");
    debug_status_ingress_fast[0] = 1'b1;  // out_ctrl0_fast = H
    debug_status_ingress_slow[0] = 1'b1;  // out_ctrl0_slow = H
    wait_cycle_fast(1010);  // 反映待ち (slow clock用に十分待つ + セレクタ遅延)
    check_pin_q(1'b1, 1'b1, 1'b1, "H,H");

    $display("");
    $display("=== mxfs_fast_slow_pattern_b_tb FINISH: ALL PASS ===");
    $finish;
  end
endmodule
