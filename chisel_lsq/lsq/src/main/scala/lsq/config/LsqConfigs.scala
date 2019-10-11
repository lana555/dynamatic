package lsq.config

import chisel3._
import chisel3.internal.firrtl.Width
import chisel3.util._

import scala.math._

case class LsqConfigs(
                       dataWidth: Int,
                       addrWidth: Int,
                       fifoDepth: Int,
                       numLoadPorts: Int,
                       numStorePorts: Int,
                       numBBs: Int,
                       loadOffsets: List[List[Int]],
                       storeOffsets: List[List[Int]],
                       loadPorts: List[List[Int]],
                       storePorts: List[List[Int]],
                       numLoads: List[Int],
                       numStores: List[Int],
                       accessType: String,
                       bufferDepth : Int,
                       name : String
){

  def loadPortIdWidth: Width = max(1, log2Ceil(numLoadPorts)).W
  def storePortIdWidth: Width = max(1, log2Ceil(numStorePorts)).W
  def fifoIdxWidth: Width = log2Ceil(fifoDepth).W
  def bufferIdxWidth: Width = max(1, log2Ceil(bufferDepth)).W
}