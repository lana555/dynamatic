#include <cmath>
#include <sstream>

#include "HlsLogging.h"
#include "HlsVhdlTb.h"

namespace hls_verify {
    const string LOG_TAG = "VVER";


    string vhdlLibraryHeader = "library IEEE;\n"
            "use ieee.std_logic_1164.all;\n"
            "use ieee.std_logic_arith.all;\n"
            "use ieee.std_logic_unsigned.all;\n"
            "use ieee.std_logic_textio.all;\n"
            "use ieee.numeric_std.all;\n"
            "use std.textio.all;\n\n"
            "use work.sim_package.all;\n"
            "\n\n";

    string commonTbBody =
            "\n"
            "----------------------------------------------------------------------------\n"
            "generate_sim_done_proc : process\n"
            "begin\n"
            "	while (transaction_idx /= TRANSACTION_NUM) loop\n"
            "		wait until tb_clk'event and tb_clk = '1';\n"
            "	end loop;\n"
            "	wait until tb_clk'event and tb_clk = '1';\n"
            "	wait until tb_clk'event and tb_clk = '1';\n"
            "	wait until tb_clk'event and tb_clk = '1';\n"
            "	assert false report \"simulation done!\" severity note;\n"
            "	assert false report \"NORMAL EXIT (note: failure is to force the simulator to stop)\" severity failure;\n"
            "	wait;\n"
            "end process;\n"
            "\n"
            "----------------------------------------------------------------------------\n"
            "gen_clock_proc : process\n"
            "begin\n"
            "	tb_clk <= '0';\n"
            "	while (true) loop\n"
            "		wait for HALF_CLK_PERIOD;\n"
            "		tb_clk <= not tb_clk;\n"
            "	end loop;\n"
            "	wait;\n"
            "end process;\n"
            "\n"
            "----------------------------------------------------------------------------\n"
            "gen_reset_proc : process\n"
            "begin\n"
            "	tb_rst <= '1';\n"
            "	wait for 10 ns;\n"
            "	tb_rst <= '0';\n"
            "	wait;\n"
            "end process;\n"
            "\n"
            "----------------------------------------------------------------------------\n"

            "generate_idle_signal: process(tb_clk,tb_rst)\n"
            "begin\n"
            "   if (tb_rst = '1') then\n"
            "       tb_temp_idle <= '1';\n"
            "   elsif rising_edge(tb_clk) then\n"
            "       tb_temp_idle <= tb_temp_idle;\n"
            "       if (tb_start_valid = '1') then\n"
            "           tb_temp_idle <= '0';\n"
            "       elsif(tb_end_valid = '1') then\n"
            "           tb_temp_idle <= '1';\n"
            "       end if;\n"
            "   end if;\n"
            "end process generate_idle_signal;\n"
            "\n"
            "----------------------------------------------------------------------------\n"
            "generate_start_signal : process(tb_clk, tb_rst)\n"
            "begin\n"
            "   if (tb_rst = '1') then\n"
            "       tb_start_valid <= '0';\n"
            "   elsif rising_edge(tb_clk) then\n"
            "       if (tb_temp_idle = '1' and tb_start_ready = '1' and tb_start_valid = '0') then\n"
            "           tb_start_valid <= '1';\n"
            "       else\n"
            "           tb_start_valid <= '0';\n"
            "       end if;\n"
            "   end if;\n"
            "end process generate_start_signal;\n"
            "\n"
            "----------------------------------------------------------------------------\n"
            "transaction_increment : process\n"
            "begin\n"
            "	wait until tb_rst = '0';\n"
            "	while (tb_temp_idle /= '1') loop\n"
            "		wait until tb_clk'event and tb_clk = '1';\n"
            "	end loop;\n"
            "	wait until tb_temp_idle = '0';\n"
            "\n"
            "	while (true) loop\n"
            "		while (tb_temp_idle /= '1') loop\n"
            "			wait until tb_clk'event and tb_clk = '1';\n"
            "		end loop;\n"
            "		transaction_idx := transaction_idx + 1;\n"
            "		wait until tb_temp_idle = '0';\n"
            "	end loop;\n"
            "end process;\n"
            "\n"
            "----------------------------------------------------------------------------\n\n";

    // class Constant

    Constant::Constant(string name, string type, string value) : constName(name), constType(type), constValue(value) {

    }

    // class MemElem

    string MemElem::clkPortName = "clk";
    string MemElem::rstPortName = "rst";
    string MemElem::ce0PortName = "ce0";
    string MemElem::we0PortName = "we0";
    string MemElem::dIn0PortName = "mem_din0";
    string MemElem::dOut0PortName = "mem_dout0";
    string MemElem::addr0PortName = "address0";
    string MemElem::ce1PortName = "ce1";
    string MemElem::we1PortName = "we1";
    string MemElem::dIn1PortName = "mem_din1";
    string MemElem::dOut1PortName = "mem_dout1";
    string MemElem::addr1PortName = "address1";
    string MemElem::donePortName = "done";
    string MemElem::inFileParamName = "TV_IN";
    string MemElem::outFileParamName = "TV_OUT";
    string MemElem::dataWidthParamName = "DATA_WIDTH";
    string MemElem::addrWidthParamName = "ADDR_WIDTH";
    string MemElem::dataDepthParamName = "DEPTH";

    // class HlsVhdTb

    HlsVhdlTb::HlsVhdlTb(const VerificationContext& ctx) : ctx(ctx) {
        duvName = ctx.get_vhdl_duv_entity_name();
        tleName = ctx.get_vhdl_duv_entity_name() + "_tb";
        cDuvParams = ctx.get_fuv_params();

        Constant hcp("HALF_CLK_PERIOD", "TIME", "2.00 ns");
        constants.push_back(hcp);

        int transNum = get_transaction_number_from_input();
        log_inf(LOG_TAG, "Transaction number computed : " + to_string(transNum));
        if (transNum <= 0) {
            log_err(LOG_TAG, "Invalid number of transactions detected!");
        }
        Constant tN("TRANSACTION_NUM", "INTEGER", to_string(transNum));
        constants.push_back(tN);

        for (int i = 0; i < cDuvParams.size(); i++) {
            CFunctionParameter p = cDuvParams[i];
            MemElem mElem;

            Constant inFileName(
                    "INPUT_" + p.parameter_name,
                    "STRING",
                    "\"" + get_input_filepath_for_param(p) + "\"");
            constants.push_back(inFileName);
            mElem.inFileParamValue = inFileName.constName;

            Constant outFileName(
                    "OUTPUT_" + p.parameter_name,
                    "STRING",
                    "\"" + get_output_filepath_for_param(p) + "\"");
            constants.push_back(outFileName);
            mElem.outFileParamValue = outFileName.constName;

            Constant dataWidth(
                    "DATA_WIDTH_" + p.parameter_name,
                    "INTEGER",
                    to_string(p.dt_width));
            constants.push_back(dataWidth);
            mElem.dataWidthParamValue = dataWidth.constName;


            mElem.isArray = (p.is_pointer && p.array_length > 1);

            if (mElem.isArray) {
                
                string addrwidthvalue;
                if (ctx.use_addr_width_32) {
                    addrwidthvalue = "32";
                }else{
                    addrwidthvalue = to_string(((int) ceil(log2(p.array_length))));
                }

                Constant addrWidth(
                        "ADDR_WIDTH_" + p.parameter_name,
                        "INTEGER",
                        addrwidthvalue);
                //to_string(((int) ceil(log2(p.array_length)))));
                constants.push_back(addrWidth);
                mElem.addrWidthParamValue = addrWidth.constName;
                Constant dataDepth(
                        "DATA_DEPTH_" + p.parameter_name,
                        "INTEGER",
                        to_string(p.array_length));
                constants.push_back(dataDepth);
                mElem.dataDepthParamValue = dataDepth.constName;
            }

            mElem.dIn0SignalName = p.parameter_name + "_mem_din0";
            mElem.dOut0SignalName = p.parameter_name + "_mem_dout0";
            mElem.we0SignalName = p.parameter_name + "_mem_" + mElem.we0PortName;
            mElem.ce0SignalName = p.parameter_name + "_mem_" + mElem.ce0PortName;
            mElem.addr0SignalName = p.parameter_name + "_mem_" + mElem.addr0PortName;
            mElem.dIn1SignalName = p.parameter_name + "_mem_din1";
            mElem.dOut1SignalName = p.parameter_name + "_mem_dout1";
            mElem.we1SignalName = p.parameter_name + "_mem_" + mElem.we1PortName;
            mElem.ce1SignalName = p.parameter_name + "_mem_" + mElem.ce1PortName;
            mElem.addr1SignalName = p.parameter_name + "_mem_" + mElem.addr1PortName;
            mElem.doneSignalName = "tb_temp_idle";

            memElems.push_back(mElem);
        }
    }

    string HlsVhdlTb::get_input_filepath_for_param(const CFunctionParameter& param) {
        if (param.is_input) {
            return ctx.get_input_vector_path(param);
        }
        return "";
    }

    string HlsVhdlTb::get_output_filepath_for_param(const CFunctionParameter& param) {
        if (param.is_output) {
            return ctx.get_vhdl_out_path(param);
        }
        return "";

    }

    string HlsVhdlTb::get_addr0_port_name_for_cParam(string& cParam) {
        return cParam + "_address0";
    }

    string HlsVhdlTb::get_addr1_port_name_for_cParam(string& cParam) {
        return cParam + "_address1";
    }

    string HlsVhdlTb::get_ce0_port_name_for_cParam(string& cParam) {
        return cParam + "_ce0";
    }

    string HlsVhdlTb::get_ce1_port_name_for_cParam(string& cParam) {
        return cParam + "_ce1";
    }

    string HlsVhdlTb::get_data_in0_port_name_for_cParam(string& cParam) {
        return cParam + "_din0";
    }

    string HlsVhdlTb::get_data_in1_port_name_for_cParam(string& cParam) {
        return cParam + "_din1";
    }

    string HlsVhdlTb::get_data_out0_port_name_for_cParam(string& cParam) {
        return cParam + "_dout0";
    }

    string HlsVhdlTb::get_data_out1_port_name_for_cParam(string& cParam) {
        return cParam + "_dout1";
    }

    string HlsVhdlTb::get_we0_port_name_for_cParam(string& cParam) {
        return cParam + "_we0";
    }

    string HlsVhdlTb::get_we1_port_name_for_cParam(string& cParam) {
        return cParam + "_we1";
    }

    string HlsVhdlTb::get_valid_in_port_name_for_cParam(string& cParam) {
        return cParam + "_valid_in";
    }

    string HlsVhdlTb::get_valid_out_port_name_for_cParam(string& cParam) {
        return cParam + "_valid_out";
    }

    string HlsVhdlTb::get_ready_in_port_name_for_cParam(string& cParam) {
        return cParam + "_ready_in";
    }

    string HlsVhdlTb::get_ready_out_port_name_for_cParam(string& cParam) {
        return cParam + "_ready_out";
    }

    string HlsVhdlTb::get_data_inSA_port_name_for_cParam(string& cParam) {
        return cParam + "_din";
    }

    string HlsVhdlTb::get_data_outSA_port_name_for_cParam(string& cParam) {
        return cParam + "_dout";
    }

    string HlsVhdlTb::get_library_header() {
        return vhdlLibraryHeader;
    }

    string HlsVhdlTb::get_entitiy_declaration() {
        return "entity " + tleName + " is\n\nend entity " + tleName + ";\n";
    }

    // function to get the port name in the entitiy for each paramter

    string HlsVhdlTb::get_architecture_begin() {
        stringstream arch;
        arch << "architecture behav of " << tleName << " is" << endl << endl;

        arch << "\t-- Constant declarations" << endl << endl;
        arch << get_constant_declaration() << endl;

        arch << "\t-- Signal declarations" << endl << endl;
        arch << get_signal_declaration() << endl;

        //arch << "\t-- DUV component" << endl << endl;
        //arch << get_duv_component_declaration() << endl;

        //arch << "\t-- Memory component" << endl << endl;
        //arch << get_mem_component_declration() << endl;

        //arch << "\t-- Argument component" << endl << endl;
        //arch << get_arg_component_declration() << endl;

        arch << "begin" << endl << endl;
        return arch.str();
    }

    string HlsVhdlTb::get_constant_declaration() {
        stringstream code;
        for (int i = 0; i < constants.size(); i++) {
            Constant c = constants[i];
            code << "\t" << "constant " << c.constName << " : " << c.constType << " := " << c.constValue << ";" << endl;
        }
        return code.str();
    }

    string HlsVhdlTb::get_signal_declaration() {
        stringstream code;

        code << "\tsignal tb_clk : std_logic := '0';" << endl;
        code << "\tsignal tb_rst : std_logic := '0';" << endl;

        code << "\tsignal tb_start_valid : std_logic := '0';" << endl;
        code << "\tsignal tb_start_ready : std_logic;" << endl;

        code << "\tsignal tb_end_valid : std_logic;" << endl;


        code << endl;

        for (int i = 0; i < cDuvParams.size(); i++) {
            MemElem m = memElems[i];

            if (m.isArray) {
                code << "\tsignal " << m.ce0SignalName << " : std_logic;" << endl;
                code << "\tsignal " << m.we0SignalName << " : std_logic;" << endl;
                code << "\tsignal " << m.dIn0SignalName << " : std_logic_vector(" << m.dataWidthParamValue << " - 1 downto 0);" << endl;
                code << "\tsignal " << m.dOut0SignalName << " : std_logic_vector(" << m.dataWidthParamValue << " - 1 downto 0);" << endl;
                code << "\tsignal " << m.addr0SignalName << " : std_logic_vector(" << m.addrWidthParamValue << " - 1 downto 0);" << endl << endl;

                code << "\tsignal " << m.ce1SignalName << " : std_logic;" << endl;
                code << "\tsignal " << m.we1SignalName << " : std_logic;" << endl;
                code << "\tsignal " << m.dIn1SignalName << " : std_logic_vector(" << m.dataWidthParamValue << " - 1 downto 0);" << endl;
                code << "\tsignal " << m.dOut1SignalName << " : std_logic_vector(" << m.dataWidthParamValue << " - 1 downto 0);" << endl;
                code << "\tsignal " << m.addr1SignalName << " : std_logic_vector(" << m.addrWidthParamValue << " - 1 downto 0);" << endl << endl;

             //   code << "\tsignal " << m.addr0SignalName + "_dummy" << " : std_logic_vector(31 downto 0);" << endl;
             //   code << "\tsignal " << m.addr1SignalName + "_dummy" << " : std_logic_vector(31 downto 0);" << endl;

            } else {
                if ((cDuvParams[i].is_return && cDuvParams[i].is_output) || !cDuvParams[i].is_return) {
                    code << "\tsignal " << m.ce0SignalName << " : std_logic;" << endl;
                    code << "\tsignal " << m.we0SignalName << " : std_logic;" << endl;
                    code << "\tsignal " << m.dOut0SignalName << " : std_logic_vector(" << m.dataWidthParamValue << " - 1 downto 0);" << endl;
                    code << "\tsignal " << m.dIn0SignalName << " : std_logic_vector(" << m.dataWidthParamValue << " - 1 downto 0);" << endl << endl << endl;
                }
            }
        }

        code << endl;

        code << "\tsignal tb_temp_idle : std_logic:= '1';" << endl;
        code << "\tshared variable transaction_idx : INTEGER := 0;" << endl;

        return code.str();
    }

    string HlsVhdlTb::get_memory_instance_generation() {
        stringstream code;
        for (int i = 0; i < memElems.size(); i++) {
            MemElem m = memElems[i];
            CFunctionParameter p = cDuvParams[i];
            if (m.isArray) {
                code << "mem_inst_" << p.parameter_name << ":\t entity work.two_port_RAM " << endl;
                code << "\tgeneric map(" << endl;
                code << "\t\t" << MemElem::inFileParamName << " => " << m.inFileParamValue << "," << endl;
                code << "\t\t" << MemElem::outFileParamName << " => " << m.outFileParamValue << "," << endl;
                code << "\t\t" << MemElem::dataDepthParamName << " => " << m.dataDepthParamValue << "," << endl;
                code << "\t\t" << MemElem::dataWidthParamName << " => " << m.dataWidthParamValue << "," << endl;
                code << "\t\t" << MemElem::addrWidthParamName << " => " << m.addrWidthParamValue << endl;
                code << "\t)" << endl;
                code << "\tport map(" << endl;
                code << "\t\t" << MemElem::clkPortName << " => " << "tb_clk," << endl;
                code << "\t\t" << MemElem::rstPortName << " => " << "tb_rst," << endl;
                code << "\t\t" << MemElem::ce0PortName << " => " << m.ce0SignalName << "," << endl;
                code << "\t\t" << MemElem::we0PortName << " => " << m.we0SignalName << "," << endl;
                code << "\t\t" << MemElem::addr0PortName << " => " << m.addr0SignalName << "," << endl;
                code << "\t\t" << MemElem::dOut0PortName << " => " << m.dOut0SignalName << "," << endl;
                code << "\t\t" << MemElem::dIn0PortName << " => " << m.dIn0SignalName << "," << endl;
                code << "\t\t" << MemElem::ce1PortName << " => " << m.ce1SignalName << "," << endl;
                code << "\t\t" << MemElem::we1PortName << " => " << m.we1SignalName << "," << endl;
                code << "\t\t" << MemElem::addr1PortName << " => " << m.addr1SignalName << "," << endl;
                code << "\t\t" << MemElem::dOut1PortName << " => " << m.dOut1SignalName << "," << endl;
                code << "\t\t" << MemElem::dIn1PortName << " => " << m.dIn1SignalName << "," << endl;
                code << "\t\t" << MemElem::donePortName << " => " << "tb_temp_idle" << endl;
                code << "\t);" << endl << endl;
            } else {

                if (p.is_input && !p.is_output && !p.is_return) {

                    code << "arg_inst_" << p.parameter_name << ":\t entity work.single_argument" << endl;
                    code << "\tgeneric map(" << endl;
                    code << "\t\t" << MemElem::inFileParamName << " => " << m.inFileParamValue << "," << endl;
                    code << "\t\t" << MemElem::outFileParamName << " => " << m.outFileParamValue << "," << endl;
                    code << "\t\t" << MemElem::dataWidthParamName << " => " << m.dataWidthParamValue << endl;
                    code << "\t)" << endl;
                    code << "\tport map(" << endl;
                    code << "\t\t" << MemElem::clkPortName << " => " << "tb_clk," << endl;
                    code << "\t\t" << MemElem::rstPortName << " => " << "tb_rst," << endl;
                    code << "\t\t" << MemElem::ce0PortName << " => " << "'1'" << "," << endl;
                    code << "\t\t" << MemElem::we0PortName << " => " << "'0'" << "," << endl;
                    code << "\t\t" << MemElem::dOut0PortName << " => " << m.dOut0SignalName << "," << endl;
                    code << "\t\t" << MemElem::dIn0PortName << " => " << "(others => '0')" << "," << endl;
                    code << "\t\t" << MemElem::donePortName << " => " << "tb_temp_idle" << endl;
                    code << "\t);" << endl << endl;


                }
                if (p.is_input && p.is_output && !p.is_return) {

                    code << "arg_inst_" << p.parameter_name << ":\t entity work.single_argument" << endl;
                    code << "\tgeneric map(" << endl;
                    code << "\t\t" << MemElem::inFileParamName << " => " << m.inFileParamValue << "," << endl;
                    code << "\t\t" << MemElem::outFileParamName << " => " << m.outFileParamValue << "," << endl;
                    code << "\t\t" << MemElem::dataWidthParamName << " => " << m.dataWidthParamValue << endl;
                    code << "\t)" << endl;
                    code << "\tport map(" << endl;
                    code << "\t\t" << MemElem::clkPortName << " => " << "tb_clk," << endl;
                    code << "\t\t" << MemElem::rstPortName << " => " << "tb_rst," << endl;
                    code << "\t\t" << MemElem::ce0PortName << " => " << "'1'" << "," << endl;
                    code << "\t\t" << MemElem::we0PortName << " => " << m.we0SignalName << "," << endl;
                    code << "\t\t" << MemElem::dOut0PortName << " => " << m.dOut0SignalName << "," << endl;
                    code << "\t\t" << MemElem::dIn0PortName << " => " << m.dIn0SignalName << "," << endl;
                    code << "\t\t" << MemElem::donePortName << " => " << "tb_temp_idle" << endl;
                    code << "\t);" << endl << endl;


                }

                if (!p.is_input && p.is_output && p.is_return) {

                    code << "arg_inst_" << p.parameter_name << ":\t entity work.single_argument" << endl;
                    code << "\tgeneric map(" << endl;
                    code << "\t\t" << MemElem::inFileParamName << " => " << m.inFileParamValue << "," << endl;
                    code << "\t\t" << MemElem::outFileParamName << " => " << m.outFileParamValue << "," << endl;
                    code << "\t\t" << MemElem::dataWidthParamName << " => " << m.dataWidthParamValue << endl;
                    code << "\t)" << endl;
                    code << "\tport map(" << endl;
                    code << "\t\t" << MemElem::clkPortName << " => " << "tb_clk," << endl;
                    code << "\t\t" << MemElem::rstPortName << " => " << "tb_rst," << endl;
                    code << "\t\t" << MemElem::ce0PortName << " => " << "'1'" << "," << endl;
                    code << "\t\t" << MemElem::we0PortName << " => " << "tb_end_valid" << "," << endl;
                    code << "\t\t" << MemElem::dOut0PortName << " => " << m.dOut0SignalName << "," << endl;
                    code << "\t\t" << MemElem::dIn0PortName << " => " << m.dIn0SignalName << "," << endl;
                    code << "\t\t" << MemElem::donePortName << " => " << "tb_temp_idle" << endl;
                    code << "\t);" << endl << endl;

                }

                if (!p.is_input && p.is_output && !p.is_return) {

                    code << "arg_inst_" << p.parameter_name << ":\t entity work.single_argument" << endl;
                    code << "\tgeneric map(" << endl;
                    code << "\t\t" << MemElem::inFileParamName << " => " << m.inFileParamValue << "," << endl;
                    code << "\t\t" << MemElem::outFileParamName << " => " << m.outFileParamValue << "," << endl;
                    code << "\t\t" << MemElem::dataWidthParamName << " => " << m.dataWidthParamValue << endl;
                    code << "\t)" << endl;
                    code << "\tport map(" << endl;
                    code << "\t\t" << MemElem::clkPortName << " => " << "tb_clk," << endl;
                    code << "\t\t" << MemElem::rstPortName << " => " << "tb_rst," << endl;
                    code << "\t\t" << MemElem::ce0PortName << " => " << "'1'" << "," << endl;
                    code << "\t\t" << MemElem::we0PortName << " => " << m.we0SignalName << "," << endl;
                    code << "\t\t" << MemElem::dIn0PortName << " => " << m.dIn0SignalName << "," << endl;
                    code << "\t\t" << MemElem::dOut0PortName << " => " << m.dOut0SignalName << "," << endl;
                    code << "\t\t" << MemElem::donePortName << " => " << "tb_temp_idle" << endl;
                    code << "\t);" << endl << endl;
                }
            }


        }
        return code.str();
    }

    string HlsVhdlTb::get_mem_component_declration() {
        stringstream code;
        code << "\tcomponent two_port_RAM is" << endl;
        code << "\tgeneric(" << endl;
        code << "\t\t" << MemElem::inFileParamName << " : STRING;" << endl;
        code << "\t\t" << MemElem::outFileParamName << " : STRING;" << endl;
        code << "\t\t" << MemElem::dataWidthParamName << " : INTEGER;" << endl;
        code << "\t\t" << MemElem::addrWidthParamName << " : INTEGER;" << endl;
        code << "\t\t" << MemElem::dataDepthParamName << " : INTEGER" << endl;
        code << "\t);" << endl;
        code << "\tport(" << endl;
        code << "\t\t" << MemElem::clkPortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::rstPortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::ce0PortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::we0PortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::addr0PortName << " : in std_logic_vector(" << MemElem::addrWidthParamName << " - 1 downto 0);" << endl;
        code << "\t\t" << MemElem::dOut0PortName << " : out std_logic_vector(" << MemElem::dataWidthParamName << " - 1 downto 0);" << endl;
        code << "\t\t" << MemElem::dIn0PortName << " : in std_logic_vector(" << MemElem::dataWidthParamName << " - 1 downto 0);" << endl;
        code << "\t\t" << MemElem::ce1PortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::we1PortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::addr1PortName << " : in std_logic_vector(" << MemElem::addrWidthParamName << " - 1 downto 0);" << endl;
        code << "\t\t" << MemElem::dOut1PortName << " : out std_logic_vector(" << MemElem::dataWidthParamName << " - 1 downto 0);" << endl;
        code << "\t\t" << MemElem::dIn1PortName << " : in std_logic_vector(" << MemElem::dataWidthParamName << " - 1 downto 0);" << endl;
        code << "\t\t" << MemElem::donePortName << " : in std_logic" << endl;
        code << "\t);" << endl;
        code << "\tend component two_port_RAM;" << endl;
        return code.str();
    }

    string HlsVhdlTb::get_arg_component_declration() {
        stringstream code;
        code << "\tcomponent single_argument is" << endl;
        code << "\tgeneric(" << endl;
        code << "\t\t" << MemElem::inFileParamName << " : STRING;" << endl;
        code << "\t\t" << MemElem::outFileParamName << " : STRING;" << endl;
        code << "\t\t" << MemElem::dataWidthParamName << " : INTEGER" << endl;
        code << "\t);" << endl;
        code << "\tport(" << endl;
        code << "\t\t" << MemElem::clkPortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::rstPortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::ce0PortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::we0PortName << " : in std_logic;" << endl;
        code << "\t\t" << MemElem::dIn0PortName << " : in std_logic_vector(" << MemElem::dataWidthParamName << " - 1 downto 0);" << endl;
        code << "\t\t" << MemElem::dOut0PortName << " : out std_logic_vector(" << MemElem::dataWidthParamName << " - 1 downto 0);" << endl;
        code << "\t\t" << MemElem::donePortName << " : in std_logic" << endl;
        code << "\t);" << endl;
        code << "\tend component single_argument;" << endl;
        return code.str();
    }

    string HlsVhdlTb::get_duv_instance_generation() {
        duvPortMap.push_back(pair<string, string>("clk", "tb_clk"));
        duvPortMap.push_back(pair<string, string>("rst", "tb_rst"));


        for (int i = 0; i < cDuvParams.size(); i++) {
            CFunctionParameter p = cDuvParams[i];
            MemElem m = memElems[i];

            if (m.isArray) {



                //duvPortMap.push_back(pair<string, string>(get_addr0_port_name_for_cParam(p.parameter_name) + "(" + to_string((int) ceil(log2(p.array_length)) - 1) + " downto 0)", m.addr0SignalName));
                duvPortMap.push_back(pair<string, string>(get_addr0_port_name_for_cParam(p.parameter_name), m.addr0SignalName));
                duvPortMap.push_back(pair<string, string>(get_ce0_port_name_for_cParam(p.parameter_name), m.ce0SignalName));
                duvPortMap.push_back(pair<string, string>(get_we0_port_name_for_cParam(p.parameter_name), m.we0SignalName));
                duvPortMap.push_back(pair<string, string>(get_data_in0_port_name_for_cParam(p.parameter_name), m.dOut0SignalName));
                duvPortMap.push_back(pair<string, string>(get_data_out0_port_name_for_cParam(p.parameter_name), m.dIn0SignalName));

                //duvPortMap.push_back(pair<string, string>(get_addr1_port_name_for_cParam(p.parameter_name) + "(" + to_string((int) ceil(log2(p.array_length)) - 1) + " downto 0)", m.addr1SignalName));
                duvPortMap.push_back(pair<string, string>(get_addr1_port_name_for_cParam(p.parameter_name), m.addr1SignalName));
                duvPortMap.push_back(pair<string, string>(get_ce1_port_name_for_cParam(p.parameter_name), m.ce1SignalName));
                duvPortMap.push_back(pair<string, string>(get_we1_port_name_for_cParam(p.parameter_name), m.we1SignalName));
                duvPortMap.push_back(pair<string, string>(get_data_in1_port_name_for_cParam(p.parameter_name), m.dOut1SignalName));
                duvPortMap.push_back(pair<string, string>(get_data_out1_port_name_for_cParam(p.parameter_name), m.dIn1SignalName));
            }

            if (!m.isArray && p.is_input) {
                duvPortMap.emplace_back(get_valid_in_port_name_for_cParam(p.parameter_name), "'1'");
                duvPortMap.emplace_back(get_data_inSA_port_name_for_cParam(p.parameter_name), m.dOut0SignalName);
            }

            if (!m.isArray && p.is_output && p.is_return) {
                duvPortMap.emplace_back("end_out", m.dIn0SignalName);
                duvPortMap.emplace_back("end_valid", "tb_end_valid"); ///////
                duvPortMap.emplace_back("end_ready", "'1'");
            }

            if (!m.isArray && !p.is_output && p.is_return) {
                duvPortMap.emplace_back("end_valid", "tb_end_valid"); ///////
                duvPortMap.emplace_back("end_ready", "'1'");
            }

            if (!m.isArray && p.is_output && !p.is_return) {
                duvPortMap.emplace_back(get_valid_out_port_name_for_cParam(p.parameter_name), m.we0SignalName);
                duvPortMap.emplace_back(get_data_outSA_port_name_for_cParam(p.parameter_name), m.dIn0SignalName);
                duvPortMap.emplace_back(get_ready_in_port_name_for_cParam(p.parameter_name), "'1'");
            }

        }

        duvPortMap.push_back(pair<string, string>("start_in", "(others => '0')"));
        duvPortMap.push_back(pair<string, string>("start_ready", "tb_start_ready"));
        duvPortMap.push_back(pair<string, string>("start_valid", "tb_start_valid"));

        stringstream code;
        code << "duv: \t entity work." << duvName << endl;
        code << "\t\tport map (" << endl;
        for (int i = 0; i < duvPortMap.size(); i++) {
            pair<string, string> elem = duvPortMap[i];
            code << "\t\t\t" << elem.first << " => " << elem.second << ((i < duvPortMap.size() - 1) ? "," : "") << endl;
        }
        code << "\t\t);" << endl << endl;
        return code.str();
    }

    string HlsVhdlTb::get_duv_component_declaration() {
        stringstream code;
        code << "\tcomponent " << duvName << " is" << endl;
        code << "\tport(" << endl;
        code << "\t\tclk : in std_logic;" << endl;
        code << "\t\trst : in std_logic;" << endl;

        for (int i = 0; i < cDuvParams.size(); i++) {
            CFunctionParameter p = cDuvParams[i];
            MemElem m = memElems[i];

            if (m.isArray) {
                code << "\t\t" << get_addr0_port_name_for_cParam(p.parameter_name) << " : out std_logic_vector(" << ((int) ceil(log2(p.array_length)) - 1) << " downto 0);" << endl;
                code << "\t\t" << get_ce0_port_name_for_cParam(p.parameter_name) << " : out std_logic;" << endl;
                code << "\t\t" << get_we0_port_name_for_cParam(p.parameter_name) << " : out std_logic;" << endl;
                code << "\t\t" << get_data_out0_port_name_for_cParam(p.parameter_name) << " : out std_logic_vector(" << ((int) p.dt_width - 1) << " downto 0);" << endl;
                code << "\t\t" << get_data_in0_port_name_for_cParam(p.parameter_name) << " : in std_logic_vector(" << ((int) p.dt_width - 1) << " downto 0);" << endl;

                code << "\t\t" << get_addr1_port_name_for_cParam(p.parameter_name) << " : out std_logic_vector(" << ((int) ceil(log2(p.array_length)) - 1) << " downto 0);" << endl;
                code << "\t\t" << get_ce1_port_name_for_cParam(p.parameter_name) << " : out std_logic;" << endl;
                code << "\t\t" << get_we1_port_name_for_cParam(p.parameter_name) << " : out std_logic;" << endl;
                code << "\t\t" << get_data_out1_port_name_for_cParam(p.parameter_name) << " : out std_logic_vector(" << ((int) p.dt_width - 1) << " downto 0);" << endl;
                code << "\t\t" << get_data_in1_port_name_for_cParam(p.parameter_name) << " : in std_logic_vector(" << ((int) p.dt_width - 1) << " downto 0);" << endl;

            } else {

                if (!m.isArray && p.is_input) {
                    code << "\t\t" << get_valid_in_port_name_for_cParam(p.parameter_name) << " : in std_logic;" << endl;
                    code << "\t\t" << get_data_in0_port_name_for_cParam(p.parameter_name) << " : in std_logic_vector(" << ((int) p.dt_width - 1) << " downto 0);" << endl;
                }

                if (!m.isArray && p.is_output && p.is_return) {
                    code << "\t\t" << "end_out : out std_logic_vector(" << ((int) p.dt_width - 1) << " downto 0);" << endl;
                    code << "\t\t" << "end_valid : out std_logic;" << endl;
                    code << "\t\t" << "end_ready : in std_logic;" << endl;
                }

                if (!m.isArray && !p.is_output && p.is_return) {
                    code << "\t\t" << "end_valid : out std_logic;" << endl;
                    code << "\t\t" << "end_ready : in std_logic;" << endl;
                }

                if (!m.isArray && p.is_output && !p.is_return) {
                    code << "\t\t" << get_valid_out_port_name_for_cParam(p.parameter_name) << " : out std_logic;" << endl;
                    code << "\t\t" << get_data_out0_port_name_for_cParam(p.parameter_name) << " : out std_logic_vector(" << ((int) p.dt_width - 1) << " downto 0);" << endl;
                    code << "\t\t" << get_ready_in_port_name_for_cParam(p.parameter_name) << " : in std_logic;" << endl;
                }
            }
        }
        code << "\t\tstart_valid : in std_logic;" << endl;
        code << "\t\tstart_ready : out std_logic" << endl;
        code << "\t);" << endl;
        code << "\tend component " << duvName << ";" << endl;
        return code.str();
    }

    string HlsVhdlTb::get_common_body() {
        return commonTbBody;
    }

    string HlsVhdlTb::get_architecture_end() {
        return "end architecture behav;\n";
    }

    string HlsVhdlTb::get_output_tag_generation() {
        stringstream out;
        out << "\n";
        out << "----------------------------------------------------------------------------\n";
        out << "-- Write \"[[[runtime]]]\" and \"[[[/runtime]]]\" for output transactor\n";
        for (int i = 0; i < cDuvParams.size(); i++) {
            if (cDuvParams[i].is_output) {
                out << "write_output_transactor_" << cDuvParams[i].parameter_name << "_runtime_proc : process\n";
                out << "	file fp             : TEXT;\n";
                out << "	variable fstatus    : FILE_OPEN_STATUS;\n";
                out << "	variable token_line : LINE;\n";
                out << "	variable token      : STRING(1 to 1024);\n";
                out << "\n";
                out << "begin\n";
                out << "	file_open(fstatus, fp, OUTPUT_" << cDuvParams[i].parameter_name << ", WRITE_MODE);\n";
                out << "	if (fstatus /= OPEN_OK) then\n";
                out << "		assert false report \"Open file \" & OUTPUT_" << cDuvParams[i].parameter_name << " & \" failed!!!\" severity note;\n";
                out << "		assert false report \"ERROR: Simulation using HLS TB failed.\" severity failure;\n";
                out << "	end if;\n";
                out << "	write(token_line, string'(\"[[[runtime]]]\"));\n";
                out << "	writeline(fp, token_line);\n";
                out << "	file_close(fp);\n";
                out << "	while transaction_idx /= TRANSACTION_NUM loop\n";
                out << "		wait until tb_clk'event and tb_clk = '1';\n";
                out << "	end loop;\n";
                out << "	wait until tb_clk'event and tb_clk = '1';\n";
                out << "	wait until tb_clk'event and tb_clk = '1';\n";
                out << "	file_open(fstatus, fp, OUTPUT_" << cDuvParams[i].parameter_name << ", APPEND_MODE);\n";
                out << "	if (fstatus /= OPEN_OK) then\n";
                out << "		assert false report \"Open file \" & OUTPUT_" << cDuvParams[i].parameter_name << " & \" failed!!!\" severity note;\n";
                out << "		assert false report \"ERROR: Simulation using HLS TB failed.\" severity failure;\n";
                out << "	end if;\n";
                out << "	write(token_line, string'(\"[[[/runtime]]]\"));\n";
                out << "	writeline(fp, token_line);\n";
                out << "	file_close(fp);\n";
                out << "	wait;\n";
                out << "end process;\n";
            }
        }
        out << "----------------------------------------------------------------------------\n\n";
        return out.str();
    }

    string HlsVhdlTb::generate_vhdl_testbench() {
        stringstream tbOut;
        tbOut << get_library_header() << endl;
        tbOut << get_entitiy_declaration() << endl;
        tbOut << get_architecture_begin() << endl;
        tbOut << get_duv_instance_generation() << endl;
        tbOut << get_memory_instance_generation() << endl;
        tbOut << get_output_tag_generation() << endl;
        tbOut << get_common_body() << endl;
        tbOut << get_architecture_end() << endl;
        return tbOut.str();
    }

    int HlsVhdlTb::get_transaction_number_from_input() {
        for (int i = 0; i < cDuvParams.size(); i++) {
            if (cDuvParams[i].is_input) {
                string inputPath = get_input_filepath_for_param(cDuvParams[i]);
                return get_number_of_transactions(inputPath);
            }
        }
        return -1;
    }

}
