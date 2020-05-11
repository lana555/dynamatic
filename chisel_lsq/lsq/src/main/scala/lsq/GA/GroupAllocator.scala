package lsq.GA

import chisel3._
import chisel3.util._
import lsq.config.LsqConfigs
import lsq.util._
import scala.math._

class GroupAllocator(config: LsqConfigs) extends Module {
  override def desiredName: String = "GROUP_ALLOCATOR_" + config.name

  val io = IO(new Bundle {
    // to load queue
    val bbLoadOffsets = Output(Vec(config.fifoDepth, UInt(log2Ceil(config.fifoDepth).W)))
    val bbLoadPorts = Output(Vec(config.fifoDepth, UInt(max(1, log2Ceil(config.numLoadPorts)).W)))
    val bbNumLoads = Output(UInt(max(1, log2Ceil(min(config.numLoadPorts, config.fifoDepth) + 1)).W))
    // from load queue
    val loadTail = Input(UInt(log2Ceil(config.fifoDepth).W))
    val loadHead = Input(UInt(log2Ceil(config.fifoDepth).W))
    val loadEmpty = Input(Bool())
    // to store queue
    val bbStoreOffsets = Output(Vec(config.fifoDepth, UInt(log2Ceil(config.fifoDepth).W)))
    val bbStorePorts = Output(Vec(config.fifoDepth, UInt(max(1, log2Ceil(config.numStorePorts)).W)))
    val bbNumStores = Output(UInt(max(1, log2Ceil(min(config.numStorePorts, config.fifoDepth) + 1)).W))
    // from store queue
    val storeTail = Input(UInt(log2Ceil(config.fifoDepth).W))
    val storeHead = Input(UInt(log2Ceil(config.fifoDepth).W))
    val storeEmpty = Input(Bool())
    // to load queue and store queue
    val bbStart = Output(Bool())
    // from previous interface
    val bbStartSignals = Input(Vec(config.numBBs, Bool()))
    val readyToPrevious = Output(Vec(config.numBBs, Bool()))
    // port related
    val loadPortsEnable = Output(Vec(config.numLoadPorts, Bool()))
    val storePortsEnable = Output(Vec(config.numStorePorts, Bool()))
  })
  require(config.fifoDepth > 1)

  def calculateEmptySlots(head: UInt, tail: UInt, empty: Bool): UInt = {
    val result = Wire(UInt(log2Ceil(config.fifoDepth + 1).W))
    when(empty || (head < tail)) {
      result := config.fifoDepth.U - tail + head
    }.otherwise {
      result := head - tail
    }
    result
  }

  val emptyLoadSlots: UInt = calculateEmptySlots(io.loadHead, io.loadTail, io.loadEmpty)
  val emptyStoreSlots: UInt = calculateEmptySlots(io.storeHead, io.storeTail, io.storeEmpty)
  // Ready to previous
  io.readyToPrevious := VecInit((config.numStores zip config.numLoads) map
    (x => (x._1.U <= emptyStoreSlots) && (x._2.U <= emptyLoadSlots)))
  // Allocate BB
  val possibleAllocations: Vec[Bool] = VecInit((io.readyToPrevious zip io.bbStartSignals) map (x => x._1 && x._2))
  val allocatedBBIdx: UInt = PriorityEncoder(possibleAllocations)
  // BB start
  io.bbStart := possibleAllocations.exists(x => x)

  // enable ports
  def getBasicBlockIdForPort(portID: Int, ports: List[List[Int]], numElems: List[Int]): Int = {
    (ports zip numElems).indexWhere(x => x._1.take(x._2).contains(portID))
  }

  for (i <- 0 until config.numLoadPorts) {
    val blockId = getBasicBlockIdForPort(i, config.loadPorts, config.numLoads)
    if (blockId >= 0) {
      io.loadPortsEnable(i) := (blockId.U === allocatedBBIdx) && io.bbStart
    } else {
      io.loadPortsEnable(i) := false.B
    }
  }

  for (i <- 0 until config.numStorePorts) {
    val blockId = getBasicBlockIdForPort(i, config.storePorts, config.numStores)
    if (blockId >= 0) {
      io.storePortsEnable(i) := (blockId.U === allocatedBBIdx) && io.bbStart
    } else {
      io.storePortsEnable(i) := false.B
    }
  }
  // set number of loads, number of stores, load ports and store ports

  io.bbNumLoads := 0.U((max(1, log2Ceil(min(config.numLoadPorts, config.fifoDepth) + 1)).W))
  io.bbNumStores := 0.U((max(1, log2Ceil(min(config.numStorePorts, config.fifoDepth) + 1)).W))
  io.bbLoadPorts := VecInit(Seq.fill(config.fifoDepth)(0.U(config.loadPortIdWidth)))
  io.bbStorePorts := VecInit(Seq.fill(config.fifoDepth)(0.U(config.storePortIdWidth)))
  io.bbLoadOffsets := VecInit(Seq.fill(config.fifoDepth)(0.U(config.fifoIdxWidth)))
  io.bbStoreOffsets := VecInit(Seq.fill(config.fifoDepth)(0.U(config.fifoIdxWidth)))

  when(io.bbStart) {
    io.bbNumLoads := MuxLookup(allocatedBBIdx, 0.U, config.numLoads.zipWithIndex map (x => x._2.U -> x._1.U))
    io.bbNumStores := MuxLookup(allocatedBBIdx, 0.U, config.numStores.zipWithIndex map (x => x._2.U -> x._1.U))
    io.bbLoadPorts := MuxLookup(
      allocatedBBIdx,
      VecInit(Seq.fill(config.fifoDepth)(0.U(config.loadPortIdWidth))),
      config.loadPorts.zipWithIndex map (x => x._2.U -> VecInit(x._1 map (y => y.U(config.loadPortIdWidth))))
    )
    io.bbStorePorts := MuxLookup(
      allocatedBBIdx,
      VecInit(Seq.fill(config.fifoDepth)(0.U(config.storePortIdWidth))),
      config.storePorts.zipWithIndex map (x => x._2.U -> VecInit(x._1 map (y => y.U(config.storePortIdWidth))))
    )
    // set corresponding BB offsets
    io.bbLoadOffsets := MuxLookup(
      allocatedBBIdx,
      VecInit(Seq.fill(config.fifoDepth)(0.U(config.fifoIdxWidth))),
      config.loadOffsets.zipWithIndex map (x => x._2.U -> VecInit(x._1 map (y =>
        addMod(y.U, io.storeTail + config.fifoDepth.U - 1.U, config.fifoDepth.U).asTypeOf(UInt(config.fifoIdxWidth))))))
    io.bbStoreOffsets := MuxLookup(
      allocatedBBIdx,
      VecInit(Seq.fill(config.fifoDepth)(0.U(config.fifoIdxWidth))),
      config.storeOffsets.zipWithIndex map (x => x._2.U -> VecInit(x._1 map (y =>
        addMod(y.U, io.loadTail + config.fifoDepth.U - 1.U, config.fifoDepth.U).asTypeOf(UInt(config.fifoIdxWidth))))))
  }
}
