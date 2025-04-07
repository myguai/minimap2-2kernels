module ilog2_32(
	input 	[31:0]	data_in,
	output 	[4:0]	pos_out
);
 
//mark:assign不能给reg赋值,只能赋给wire
wire [4:0]  data_chk;
wire [15:0] data_1;	
wire [7:0]  data_2;
wire [3:0]  data_3;
wire [1:0]  data_4;
 
 
assign data_chk[4] = |data_in[31:16];//高16位相或,依此类推
assign data_chk[3] = |data_1[15:8];
assign data_chk[2] = |data_2[7:4];
assign data_chk[1] = |data_3[3:2];
assign data_chk[0] = |data_4[1]; 
 
assign	data_1	= (data_chk[4]) ? data_in[31:16] : data_in[15:0]; //若data_in高16位有1,则data1取其高16位,否则取低16位
assign	data_2 	= (data_chk[3]) ? data_1[15:8] 	: data_1[7:0];		//若data_1高8位有1,则data2取其高8位,否则取低8位
assign	data_3 	= (data_chk[2]) ? data_2[7:4] : data_2[3:0];		//若data_2高4位有1,则data3取其高4位,否则取低4位
assign	data_4 	= (data_chk[1]) ? data_3[3:2] : data_3[1:0];		//若data_3高2位有1,则data4取其高2位,否则取低2位
assign 	pos_out = data_chk;


endmodule
