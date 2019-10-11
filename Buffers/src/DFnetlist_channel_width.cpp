#include <cassert>
#include "DFnetlist.h"

using namespace Dataflow;

void DFnetlist_Impl::inferChannelWidth(int default_width)
{
    // First define the known widths
    ForAllBlocks(b) {
        auto t = getBlockType(b);

        // Input port of a constant should be control.
        if (t == CONSTANT) {
            portID p = getInPort(b);
            if (validPort(p) and getPortWidth(p) < 0) setPortWidth(p, 0);
            continue;
        }

        // Conditional port of branch and select should be boolean
        if (t == BRANCH or t == SELECT) {
            ForAllInputPorts(b, p) {
                if (getPortType(p) == SELECTION_PORT) {
                    if (getPortWidth(p) < 0) setPortWidth(p, 1);
                    break;
                }
            }
            continue;
        }

        if (t != DEMUX) continue;

        // Now we have a demux. All input ports are control,
        // except the one for data.
        portID d = getDataPort(d);
        ForAllInputPorts(b, p) {
            if (p != d and getPortWidth(p) < 0) setPortWidth(p, 0);
        }
    }


    // Iterative process to infer the widths
    bool changes = false;
    do {
        changes = false;
        ForAllBlocks(b) {
            auto t = getBlockType(b);
            portID p_in = getInPort(b);
            portID p_out = getOutPort(b);
            int w_in = validPort(p_in) ? getPortWidth(p_in) : -1;
            int w_out = validPort(p_out) ? getPortWidth(p_out) : -1;
            int w, w_data;
            portID d;

            // Get selection port (if any)
            portID select = invalidDataflowID;
            ForAllInputPorts(b, p) {
                if (getPortType(p) == SELECTION_PORT) {
                    select = p;
                    break;
                }
            }

            switch (t) {

                // No propagation for next four blocks
            case OPERATOR:
            case CONSTANT:
            case FUNC_ENTRY:
            case FUNC_EXIT:
            case SINK:
            case SOURCE:
            case MUX:
            case CNTRL_MG:
            case LSQ:
            case MC:
                break;

            case ELASTIC_BUFFER:
                if (w_in < 0 and w_out >= 0) {
                    setPortWidth(p_in, w_out);
                    changes = true;
                } else if (w_in >= 0 and w_out < 0) {
                    setPortWidth(p_out, w_in);
                    changes = true;
                }

            case FORK:
                // Forward
                if (w_in >= 0) {
                    ForAllOutputPorts(b, p) {
                        if (getPortWidth(p) < 0) {
                            setPortWidth(p, w_in);
                            changes = true;
                        }
                    }
                } else {
                    // Backward
                    w = -1;
                    ForAllOutputPorts(b, p) w = max(w, getPortWidth(p));
                    if (w >= 0) {
                        setPortWidth(p_in, w);
                        changes = true;
                    }
                }

            case MERGE:
                // Backward
                if (w_out >= 0) {
                    ForAllInputPorts(b, p) {
                        if (getPortWidth(p) < 0) {
                            setPortWidth(p, w_out);
                            changes = true;
                        }
                    }
                } else {
                    // Forward
                    w = -1;
                    ForAllInputPorts(b, p) w = max(w, getPortWidth(p));
                    if (w >= 0) {
                        setPortWidth(p_out, w);
                        changes = true;
                    }
                }
                break;

            case BRANCH:
            case DEMUX:
                // We know all control and conditional ports have been defined
                // We first get the data port.
                if (t == DEMUX) d = getDataPort(b);
                else {
                    // One is SELECTION and the other is GENERIC
                    ForAllInputPorts(b, p) {
                        if (p != select) {
                            d = p;
                            break;
                        }
                    }
                }

                w_data = getPortWidth(d);
                if (w_data >= 0) {
                    // Propagate forward
                    ForAllOutputPorts(b, p) {
                        if (getPortWidth(p) == -1) {
                            setPortWidth(p, w_data);
                            changes = true;
                        }
                    }
                } else {
                    // Propagate backward
                    w = -1;
                    ForAllOutputPorts(b, p) w = max(w, getPortWidth(p));
                    if (w >= 0) {
                        setPortWidth(d, w);
                        changes = true;
                    }
                }
                break;

            case SELECT:
                d = getOutPort(b);
                w_data = getPortWidth(d);
                if (w_data >= 0) {
                    // Propagate backward
                    // We know that the condition port has already been defined.
                    ForAllInputPorts(b, p) {
                        if (getPortWidth(p) == -1) {
                            setPortWidth(p, w_data);
                            changes = true;
                        }
                    }
                } else {
                    // Propagate forward
                    w = -1;
                    ForAllInputPorts(b, p) {
                        if (p == select) continue;
                        w = max(w, getPortWidth(p));
                    }
                    if (w >= 0) {
                        setPortWidth(d, w);
                        changes = true;
                    }
                }
                break;

            case UNKNOWN:
            default:
                assert(false);
            }
        }

        // Propagate through channels
        ForAllChannels(c) {
            portID src = getSrcPort(c);
            portID dst = getDstPort(c);
            int w_src = getPortWidth(src);
            int w_dst = getPortWidth(dst);
            if (getPortWidth(src) == getPortWidth(dst)) continue;
            if (w_src == -1) {
                setPortWidth(src, w_dst);
                changes = true;
            } else if (w_dst == -1) {
                setPortWidth(dst, w_src);
                changes = true;
            }
        }

    } while (changes);


    ForAllBlocks(b) {
        ForAllPorts(b, p) {
            if (getPortWidth(p) == -1) setPortWidth(p, default_width);
        }
    }
}
