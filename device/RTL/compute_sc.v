module compute_sc(
    input   clock,
    input   resetn,

    input   ivalid,
    output  oready,

    output reg  ovalid,
    input   iready,

    input   [63:0]  a_x,
    input   [31:0]  a_y,
    input   [63:0]  ri,
    input   [31:0]  qi,
	input	[31:0]  q_span,
    input   [31:0]  avg_qspan,
    input   [31:0]  max_dist_x,
    input   [31:0]  max_dist_y,
    input   [31:0]  bw,
    output reg [31:0]  data_return

);


wire    [63:0]  dr;
wire    [31:0]  dq;

wire    [31:0]  dd_pre;
wire    [31:0]  dd;


wire direct_return;
wire direct_return_0, direct_return_1, direct_return_2;
wire branch_extra;


wire    [31:0]  min_d;
wire    [31:0]  sc;
wire    [4:0]   c_log;
wire    [15:0]  c_lin;
wire    [31:0]  c_lin_pre;
wire	  [31:0]	 c_lin_final;
wire	  			 c_lin_round;
wire    [31:0]  branch_data_part;

wire    [31:0]  branch_data;


//stage 0 register
reg     [31:0]  dr_stage_0;
reg     [31:0]  dq_stage_0;
reg     [31:0]  max_dist_x_stage_0;
reg     [31:0]  max_dist_y_stage_0;
reg     [31:0]  bw_stage_0;
reg     [7:0]   n_segs_stage_0;
reg     [7:0]   q_span_stage_0;
reg     [31:0]  avg_qspan_stage_0;
reg             data_valid_stage_0;
reg				 branch_extra_stage_0;


//stage 1 register
reg     [31:0]  dd_stage_1;
reg             direct_return_0_stage_1;
reg             direct_return_1_stage_1;
reg     [31:0]  min_d_stage_1;
reg     [31:0]  bw_stage_1;
reg     [7:0]   q_span_stage_1;
reg     [31:0]  avg_qspan_stage_1;
reg             data_valid_stage_1;

//stage 2 register
reg     [31:0]  sc_stage_2;
reg             direct_return_stage_2;
reg     [4:0]   c_log_stage_2;
reg     [31:0]  c_lin_pre_stage_2;
reg             data_valid_stage_2;

//stage 3 register
reg     [31:0]  sc_stage_3;
reg     [31:0]  branch_data_part_stage_3;
reg             direct_return_stage_3;
reg             data_valid_stage_3;


//-----------------------stage 0 logic-----------------------------------------------------//
// lets assume dr will not be larger than 32768
assign dr = ri - a_x;  //try to put to dsp
assign dq = qi - a_y; //try to put to dsp

// if dr is larger than max_dist_x, direct return
assign branch_extra = (ri > (a_x + max_dist_x));

always @(posedge clock or negedge resetn) begin
    if(~resetn)begin
        dr_stage_0 <= 32'b0;
        dq_stage_0 <= 32'b0;
        max_dist_x_stage_0 <= 32'b0;
        max_dist_y_stage_0 <= 32'b0;
        bw_stage_0 <= 32'b0;
        q_span_stage_0 <= 8'b0;
        avg_qspan_stage_0 <= 32'b0;
        data_valid_stage_0 <= 1'b0;
		  branch_extra_stage_0 <= 1'b0;
    end
    else begin
        dr_stage_0 <= dr[31:0];
        dq_stage_0 <= dq;
        max_dist_x_stage_0 <= max_dist_x;
        max_dist_y_stage_0 <= max_dist_y;
        bw_stage_0 <= bw;
        q_span_stage_0 <= q_span[7:0];
        avg_qspan_stage_0 <= avg_qspan;
        data_valid_stage_0 <= ivalid;
		  branch_extra_stage_0 <= branch_extra;
    end
end
//----------------------stage 1 logic----------------------------------------------------

assign dd_pre = dr_stage_0[31:0] - dq_stage_0;
assign dd = dd_pre[31]?((~dd_pre)+1):dd_pre;//signal bit

assign direct_return_0 = ((dr_stage_0[31]!=1) && (dr_stage_0[31:0]>max_dist_y_stage_0)) || (|(dr_stage_0)==1'b0) || branch_extra_stage_0;
assign direct_return_1 = (dq_stage_0[31]==1 || |(dq_stage_0)==0  || dq_stage_0 > max_dist_x_stage_0 || (dq_stage_0 > max_dist_y_stage_0));

assign min_d = dd_pre[31]?dr_stage_0[31:0]:dq_stage_0;


always @(posedge clock or negedge resetn) begin
    if(~resetn)begin
        dd_stage_1 <= 32'b0;
        direct_return_0_stage_1 <= 1'b0;
        direct_return_1_stage_1 <= 1'b0;
        min_d_stage_1 <= 32'b0;
        bw_stage_1 <= 32'b0;
        q_span_stage_1 <= 8'b0;
        avg_qspan_stage_1 <= 32'b0;
        data_valid_stage_1 <= 1'b0;
    end
    else begin
        dd_stage_1 <= dd;
        direct_return_0_stage_1 <= direct_return_0;
        direct_return_1_stage_1 <= direct_return_1;
        min_d_stage_1 <= min_d;
        bw_stage_1 <= bw_stage_0;
        q_span_stage_1 <= q_span_stage_0;
        avg_qspan_stage_1 <= avg_qspan_stage_0;
        data_valid_stage_1 <= data_valid_stage_0;
    end
end

//------------------------stage 2 logic-------------------------------------------------

assign direct_return_2 =  (dd_stage_1>bw_stage_1);
assign direct_return = direct_return_0_stage_1 | direct_return_1_stage_1 |direct_return_2;


assign sc = (q_span_stage_1 > min_d_stage_1)||min_d_stage_1[31]?min_d_stage_1:q_span_stage_1;
ilog2_32 u_ilog2(dd_stage_1,c_log);



assign c_lin_pre = (dd_stage_1[17:0] * avg_qspan_stage_1[17:0]); //assume it is fixed point



always @(posedge clock or negedge resetn) begin
    if(!resetn)begin
        sc_stage_2 <= 32'b0;
        direct_return_stage_2 <= 1'b0;
        c_log_stage_2 <= 5'b0;
        c_lin_pre_stage_2 <= 32'b0;
        data_valid_stage_2 <= 1'b0;
    end
    else begin
        sc_stage_2 <= sc;
        direct_return_stage_2 <= direct_return;
        c_log_stage_2 <= c_log;
        c_lin_pre_stage_2 <= c_lin_pre;
        data_valid_stage_2 <= data_valid_stage_1;
    end
end

//---------------------stage 3 logic--------------------------------

assign c_lin = c_lin_pre_stage_2[31:19];
assign c_lin_round = (&c_lin_pre_stage_2[18:7]);					//round the data

assign c_lin_final = c_lin + c_lin_round;
assign branch_data_part = c_lin_final + (c_log_stage_2>>1);


always @(posedge clock or negedge resetn) begin
    if(~resetn)begin
        sc_stage_3 <= 32'b0;
        branch_data_part_stage_3 <= 32'b0;
        direct_return_stage_3 <= 1'b0;
        data_valid_stage_3 <= 1'b0;
    end
    else begin
        sc_stage_3 <= sc_stage_2;
        branch_data_part_stage_3 <= branch_data_part;
        direct_return_stage_3 <= direct_return_stage_2;
        data_valid_stage_3 <= data_valid_stage_2;
    end
end


//-----------------------------stage 4 logic----------------------


assign branch_data = sc_stage_3 - branch_data_part_stage_3;



always @(posedge clock or negedge resetn) begin
    if(~resetn)begin
        data_return <= 32'b0;

    end
    else begin
        if(direct_return_stage_3)begin
            data_return <= 32'h80000001;
        end
        else begin
            data_return <= branch_data;
        end
            
    end
end

always @(posedge clock or negedge resetn) begin
    if(~resetn)begin
        ovalid <= 1'b0;
    end
    else begin
        ovalid <= data_valid_stage_3;
    end
end

// ready data is always 1, no stall
assign oready=1;

endmodule